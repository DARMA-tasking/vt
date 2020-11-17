/*
//@HEADER
// *****************************************************************************
//
//                                  counter.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_METER_COUNTER_H
#define INCLUDED_VT_RUNTIME_COMPONENT_METER_COUNTER_H

#include "vt/config.h"
#include "vt/runtime/component/meter/stats_pack.h"

namespace vt { namespace runtime { namespace component { namespace meter {

/**
 * \struct Counter
 *
 * \brief Diagnostic that counts some quantity over time.
 */
template <typename T>
struct Counter : DiagnosticMeter {

  /**
   * \brief Default constructor available for ease of putting this type in a
   * class. But, all valid ways to construction involve the factory methods in
   * the \c Diagnostic base class for component
   */
  Counter() = default;

private:
  explicit Counter(detail::DiagnosticValue<T>* in_impl)
    : impl_(in_impl)
  { }

  friend struct component::Diagnostic;

public:
  /**
   * \brief Increment the counter
   *
   * \param[in] val amount to increment
   */
  void increment(T val = 1) {
#   if vt_check_enabled(diagnostics)
    if (impl_) {
      impl_->update(val);
    }
#   endif
  }

  /**
   * \brief Decrement the counter
   *
   * \param[in] val amount to decrement
   */
  void decrement(T val = 1) {
#   if vt_check_enabled(diagnostics)
    if (impl_) {
      impl_->update(-val);
    }
#   endif
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | impl_;
  }

private:
  detail::DiagnosticValue<T>* impl_ = nullptr; /**< The actual underlying value */
};

}}}} /* end namespace vt::runtime::component::meter */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_METER_COUNTER_H*/
