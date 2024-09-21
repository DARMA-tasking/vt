/*
//@HEADER
// *****************************************************************************
//
//                            keyval_printer.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_ERROR_KEYVAL_PRINTER_IMPL_H
#define INCLUDED_VT_CONFIGS_ERROR_KEYVAL_PRINTER_IMPL_H

#include "vt/configs/error/keyval_printer.h"
#include "vt/configs/debug/debug_colorize.h"

#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <string>
#include <vector>

#include INCLUDE_FMT_CORE

namespace vt { namespace util { namespace error {

template <typename T, typename U>
static std::string makeVal(T const& t, U const& u) {
  auto green           = ::vt::debug::green();
  auto blue            = ::vt::debug::blue();
  auto reset           = ::vt::debug::reset();
  auto bred            = ::vt::debug::bred();
  auto magenta         = ::vt::debug::magenta();
  return ::fmt::format("{}{:>40}{} = {}{}{}",magenta,t,reset,green,u,reset);
}

template <typename ConsT, typename ConsU>
/*static*/ typename PrinterNameValue<0,ConsT,ConsU>::VectorType
PrinterNameValue<0,ConsT,ConsU>::make(ConsT const& names, ConsU const& values) {
  return VectorType{makeVal(std::get<0>(names),std::get<0>(values))};
}

template <std::size_t cur, typename ConsT, typename ConsU>
/*static*/ typename PrinterNameValue<cur,ConsT,ConsU>::VectorType
PrinterNameValue<cur,ConsT,ConsU>::make(ConsT const& names, ConsU const& values) {
  auto vec = PrinterNameValue<cur-1,ConsT,ConsU>::make(names,values);
  vec.emplace_back(makeVal(std::get<cur>(names),std::get<cur>(values)));
  return vec;
}

}}} /* end namespace vt::util::error */

#endif /*INCLUDED_VT_CONFIGS_ERROR_KEYVAL_PRINTER_IMPL_H*/
