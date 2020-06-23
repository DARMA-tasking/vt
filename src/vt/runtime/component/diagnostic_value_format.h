/*
//@HEADER
// *****************************************************************************
//
//                          diagnostic_value_format.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_FORMAT_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_FORMAT_H

#include "vt/runtime/component/diagnostic_value.h"

#include <fmt/format.h>

namespace vt { namespace runtime { namespace component { namespace detail {

template <typename T, typename _Enable = void>
struct DiagnosticValueFormatter;

template <typename T>
struct DiagnosticValueFormatter<
  T, typename std::enable_if_t<
       std::is_same<T, double>::value or std::is_same<T, float>::value
     >
> {
  static constexpr char const* format = "{:.2f}";
};

template <typename T>
struct DiagnosticValueFormatter<
  T, typename std::enable_if_t<
       not (std::is_same<T, double>::value or std::is_same<T, float>::value)
     >
> {
  static constexpr char const* format = "{}";
};

template <typename T>
struct DiagnosticStringizer {

  static DiagnosticString get(DiagnosticValueWrapper<T> wrapper) {
    std::string const spec = DiagnosticValueFormatter<T>::format;

    DiagnosticString dstr;
    dstr.min_value_ = fmt::format(spec, wrapper.min());
    dstr.max_value_ = fmt::format(spec, wrapper.max());
    dstr.sum_value_ = fmt::format(spec, wrapper.sum());
    dstr.avg_value_ = fmt::format("{:.2f}", wrapper.avg());
    dstr.std_value_ = fmt::format("{:.2f}", wrapper.stdv());
    dstr.var_value_ = fmt::format("{:.2f}", wrapper.var());
    return dstr;
  }

};

}}}} /* end namespace vt::runtime::component::detail */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_FORMAT_H*/
