/*
//@HEADER
// *****************************************************************************
//
//                            assert_out_info.impl.h
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

#if !defined INCLUDED_VT_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H
#define INCLUDED_VT_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H

#include "vt/configs/error/common.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/assert_out.h"
#include "vt/configs/error/assert_out_info.h"
#include "vt/configs/error/keyval_printer.h"
#include "vt/configs/debug/debug_colorize.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <string>
#include <sstream>

#include "fmt/core.h"

namespace vt { namespace debug { namespace assert {

template <typename... Args, typename... Args2>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOutInfo(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args2...>&& tup, std::tuple<Args...>&& t2
) {
  return assertOut(
    fail,cond,str,file,line,func,error,std::forward<std::tuple<Args...>>(t2)
  );
}

template <typename... Args, typename... Args2>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOutInfo(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args2...>&& t1, std::tuple<Args...>&& t2
) {
  using KeyType = std::tuple<Args2...>;
  using ValueType = std::tuple<Args...>;
  static constexpr auto size = std::tuple_size<KeyType>::value;
  using PrinterType = util::error::PrinterNameValue<size-1,KeyType,ValueType>;

  // Output the standard assert message
  assertOut(false,cond,str,file,line,func,error,std::make_tuple());

  // Output each expression computed passed to the function along with the
  // computed value of that passed expression
  auto varlist = PrinterType::make(t1,t2);

  auto node      = debug::preNode();
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  auto node_str  = debug::proc(node);
  auto prefix    = vt_pre + node_str + " ";
  auto byellow   = debug::byellow();
  auto yellow    = debug::yellow();
  auto reset     = debug::reset();
  auto green     = debug::green();
  auto seperator = fmt::format("{}{}{:-^120}{}\n", prefix, yellow, "", reset);
  auto space     = fmt::format("{}\n", prefix);
  auto title     = fmt::format(
    "{}{}{:-^120}{}\n", prefix, yellow, " Debug State Assert Info ", reset
  );
  auto pre       = fmt::format("{}{}{}{}", seperator, title, seperator, space);

  std::ostringstream str_buf;

  auto buf = fmt::format("{}", pre);
  str_buf << buf;

  //str_buf << seperator << seperator;
  for (auto&& var : varlist) {
    auto cur = fmt::format("{}{}{:>25}{}\n", prefix, green, var, reset);
    str_buf << cur;
  }
  str_buf << space << seperator << seperator;
  ::vt::output(str_buf.str(),error,false,false,true,fail);

  if (fail) {
    vtAbort("Assertion failed");
  }
}

}}} /* end namespace vt::debug::assert */

#endif /*INCLUDED_VT_CONFIGS_ERROR_ASSERT_OUT_INFO_IMPL_H*/
