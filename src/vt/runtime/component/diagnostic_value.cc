/*
//@HEADER
// *****************************************************************************
//
//                             diagnostic_value.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/runtime/component/diagnostic.h"
#include "vt/runtime/component/diagnostic_value_format.h"
#include "vt/messaging/message.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/pipe/pipe_manager.h"

#include <limits>

namespace vt { namespace runtime { namespace component { namespace detail {

namespace {

template <typename T>
void reduceHelper(
  Diagnostic* diagnostic, DiagnosticErasedValue* out, T val, DiagnosticUnit unit,
  DiagnosticUpdate update
) {
  using ValueType = DiagnosticValueWrapper<T>;
  using ReduceMsgType = collective::ReduceTMsg<ValueType>;

  // Get the reducer from the component
  auto r = diagnostic->reducer();
  auto msg = makeMessage<ReduceMsgType>(
    ValueType{typename ValueType::ReduceTag{}, val}
  );
  auto cb = theCB()->makeFunc<ReduceMsgType>([=](ReduceMsgType* m) {
    auto& reduced_val = m->getConstVal();
    *out = DiagnosticEraser<T>::get(reduced_val);
    out->hist_ = reduced_val.getHistogram();
    out->update_ = update;
    out->unit_ = unit;
    if (update == DiagnosticUpdate::Min) {
      out->is_valid_value_ = reduced_val.min() != std::numeric_limits<T>::max();
    } else {
      out->is_valid_value_ = reduced_val.sum() != 0;
    }
  });
  r->reduce<collective::PlusOp<ValueType>>(0, msg.get(), cb);
}

} /* end anon namespace */

/*
 * These are very purposely explicitly instantiated to avoid complex header
 * dependencies and longer compile times. The possible reduce types are limited
 * for diagnostic values and including this file would require a dependency
 * between every component and reduce/pipe for the reduction/callback.
 */
#define DIAGNOSIC_VALUE_INSTANCE(TYPE)                                  \
  template <>                                                           \
  void DiagnosticValue<TYPE>::reduceOver(                               \
    Diagnostic* diagnostic, DiagnosticErasedValue* out, int snapshot    \
  ) {                                                                   \
    reduceHelper(                                                       \
      diagnostic, out, values_[snapshot].getComputedValue(), getUnit(), \
      getUpdateType()                                                   \
    );                                                                  \
  }                                                                     \


DIAGNOSIC_VALUE_INSTANCE(int64_t)
DIAGNOSIC_VALUE_INSTANCE(int32_t)
DIAGNOSIC_VALUE_INSTANCE(int16_t)
DIAGNOSIC_VALUE_INSTANCE(int8_t)
DIAGNOSIC_VALUE_INSTANCE(uint64_t)
DIAGNOSIC_VALUE_INSTANCE(uint32_t)
DIAGNOSIC_VALUE_INSTANCE(uint16_t)
DIAGNOSIC_VALUE_INSTANCE(uint8_t)
DIAGNOSIC_VALUE_INSTANCE(double)
DIAGNOSIC_VALUE_INSTANCE(float)

}}}} /* end namespace vt::runtime::component::detail */
