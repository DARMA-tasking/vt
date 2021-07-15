/*
//@HEADER
// *****************************************************************************
//
//                              assert_out.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_ERROR_ASSERT_OUT_IMPL_H
#define INCLUDED_VT_CONFIGS_ERROR_ASSERT_OUT_IMPL_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/assert_out.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/error/pretty_print_message.h"

#include <tuple>
#include <type_traits>
#include <string>
#include <cassert>

#include "fmt/core.h"

namespace vt { namespace debug { namespace assert {

template <typename>
inline void assertOutExpr(
  bool fail, std::string const cond, std::string const& file, int const line,
  std::string const& func, ErrorCodeType error
) {
  auto msg = "Assertion failed:";
  auto reason = "";
  auto assert_fail_str = stringizeMessage(msg,reason,cond,file,line,func,error);
  ::vt::output(assert_fail_str,error,true,true,true,fail);
  if (fail) {
    vtAbort("Assertion failed");
  }
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args...>&& tup
) {
  auto msg = "Assertion failed:";
  auto assert_fail_str = stringizeMessage(msg,str,cond,file,line,func,error);
  ::vt::output(assert_fail_str,error,true,true,true,fail);
  if (fail) {
    vtAbort("Assertion failed");
  }
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOutImpl(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, Args&&... args
) {
  auto const arg_str = ::fmt::format(str,std::forward<Args>(args)...);
  assertOut(false,cond,arg_str,file,line,func,error,std::make_tuple());
  if (fail) {
    vtAbort("Assertion failed");
  }
}

template <typename Tuple, size_t... I>
inline void assertOutImplTup(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, Tuple&& tup, std::index_sequence<I...>
) {
  assertOutImpl(
    fail,cond,str,file,line,func,error,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(tup)
    )...
  );
}

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args...>&& tup
) {
  static constexpr auto size = std::tuple_size<std::tuple<Args...>>::value;
  assertOutImplTup(
    fail,cond,str,file,line,func,error,
    std::forward<std::tuple<Args...>>(tup),
    std::make_index_sequence<size>{}
  );
}

}}} /* end namespace vt::debug::assert */

#endif /*INCLUDED_VT_CONFIGS_ERROR_ASSERT_OUT_IMPL_H*/
