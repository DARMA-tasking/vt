/*
//@HEADER
// *****************************************************************************
//
//                                 assert_out.h
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

#if !defined INCLUDED_CONFIGS_ERROR_ASSERT_OUT_H
#define INCLUDED_CONFIGS_ERROR_ASSERT_OUT_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/error/common.h"

#include <tuple>
#include <type_traits>
#include <string>

namespace vt { namespace debug { namespace assert {

template <typename=void>
inline void assertOutExpr(
  bool fail, std::string const cond, std::string const& file, int const line,
  std::string const& func, ErrorCodeType error
);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args...>&& args
);

template <typename... Args>
inline
std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
assertOut(
  bool fail, std::string const cond, std::string const& str,
  std::string const& file, int const line, std::string const& func,
  ErrorCodeType error, std::tuple<Args...>&& args
);

}}} /* end namespace vt::debug::assert */

#include "vt/configs/error/assert_out.impl.h"

#endif /*INCLUDED_CONFIGS_ERROR_ASSERT_OUT_H*/
