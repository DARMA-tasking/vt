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

namespace vt { namespace runtime { namespace component { namespace detail {

namespace {

template <typename T>
void reduceHelper(Diagnostic* diagnostic, DiagnosticString* out, T val) {
  using ValueType = DiagnosticValueWrapper<T>;
  using ReduceMsgType = collective::ReduceTMsg<ValueType>;

  // Get the reducer from the component
  auto r = diagnostic->reducer();
  auto msg = makeMessage<ReduceMsgType>(
    ValueType{typename ValueType::ReduceTag{}, val}
  );
  auto cb = theCB()->makeFunc<ReduceMsgType>([=](ReduceMsgType* m) {
    auto reduced_val = m->getConstVal();
    *out = DiagnosticStringizer<T>::get(reduced_val);
  });
  r->reduce<collective::PlusOp<ValueType>>(0, msg.get(), cb);
}

} /* end anon namespace */

template <>
void DiagnosticValue<int64_t>::reduceOver(
  Diagnostic* diagnostic, DiagnosticString* out
) {
  reduceHelper(diagnostic, out, value_.getComputedValue());
}

template <>
void DiagnosticValue<int32_t>::reduceOver(
  Diagnostic* diagnostic, DiagnosticString* out
) {
  reduceHelper(diagnostic, out, value_.getComputedValue());
}

template <>
void DiagnosticValue<int16_t>::reduceOver(
  Diagnostic* diagnostic, DiagnosticString* out
) {
  reduceHelper(diagnostic, out, value_.getComputedValue());
}

template <>
void DiagnosticValue<double>::reduceOver(
  Diagnostic* diagnostic, DiagnosticString* out
) {
  reduceHelper(diagnostic, out, value_.getComputedValue());
}

template <>
void DiagnosticValue<float>::reduceOver(
  Diagnostic* diagnostic, DiagnosticString* out
) {
  reduceHelper(diagnostic, out, value_.getComputedValue());
}

}}}} /* end namespace vt::runtime::component::detail */
