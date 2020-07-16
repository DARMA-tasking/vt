/*
//@HEADER
// *****************************************************************************
//
//                                 soft_error.h
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

#if !defined INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H
#define INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H

/*
 *  A soft error is treated like an ignored warning in certain build/runtime
 *  modes (e.g, some production cases) and an error in other modes (e.g., when
 *  debugging is enabled). In production modes, if the user configures it as so,
 *  the cost of these checks can be fully optimized out.
 */

#include "vt/configs/debug/debug_config.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/common.h"
#include "vt/configs/error/pretty_print_message.h"

#include <string>

#include "fmt/format.h"

namespace vt {


template <typename... Args>
inline std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value != 0>
warningImpl(
  std::string const& str, ErrorCodeType error, bool quit,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
) {
  auto msg = "vtWarn() Invoked";
  std::string const buf = ::fmt::format(str, std::forward<Args>(args)...);
  auto inf = debug::stringizeMessage(msg,buf,"",file,line,func,error);
  if (quit) {
    return ::vt::output(inf,error,true,true,true,true);
  } else {
    return ::vt::output(inf,error,false,true,true,true);
  }
}

template <typename... Args>
inline std::enable_if_t<std::tuple_size<std::tuple<Args...>>::value == 0>
warningImpl(
  std::string const& str, ErrorCodeType error, bool quit,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
) {
  auto msg = "vtWarn() Invoked";
  auto inf = debug::stringizeMessage(msg,str,"",file,line,func,error);
  if (quit) {
    return ::vt::output(inf,error,true,true,true,true);
  } else {
    return ::vt::output(inf,error,false,true,true,true);
  }
}

template <typename Tuple, size_t... I>
inline void warningImplTup(
  std::string const& str, ErrorCodeType error, bool quit,
  std::string const& file, int const line, std::string const& func,
  Tuple&& tup, std::index_sequence<I...>
) {
  warningImpl(
    str,error,quit,file,line,func,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(tup)
    )...
  );
}

template <typename... Args>
inline void warning(
  std::string const& str, ErrorCodeType error, bool quit,
  std::string const& file, int const line, std::string const& func,
  std::tuple<Args...>&& tup
) {
  static constexpr auto size = std::tuple_size<std::tuple<Args...>>::value;
  warningImplTup(
    str,error,quit,file,line,func,
    std::forward<std::tuple<Args...>>(tup),
    std::make_index_sequence<size>{}
  );
}

} /* end namespace vt */

#if vt_check_enabled(production)
  #define vtWarn(str)
  #define vtWarnCode(error,str)
  #define vtWarnIf(cond,str)           vt_force_use(cond)
  #define vtWarnIfCode(error,cond,str) vt_force_use(cond)
  #define vtWarnFail(str)
  #define vtWarnFailCode(error,str)
#else
  #define vtWarn(str)                                             \
    ::vt::warning(str,1,    false, DEBUG_LOCATION, std::make_tuple());
  #define vtWarnCode(code,str)                                    \
    ::vt::warning(str,code, false, DEBUG_LOCATION, std::make_tuple());
  #define vtWarnFail(str)                                         \
    ::vt::warning(str,1,    true,  DEBUG_LOCATION, std::make_tuple());
  #define vtWarnFailCode(code,str)                                \
    ::vt::warning(str,code, true,  DEBUG_LOCATION, std::make_tuple());
  #define vtWarnIf(cond,str)                                      \
    do {                                                                  \
      if (cond) {                                                         \
        vtWarn(str);                                                      \
      }                                                                   \
    } while (false)
  #define vtWarnIfCode(code,cond,str)                                     \
    do {                                                                  \
      if (cond) {                                                         \
        vtWarnCode(code,str);                                             \
      }                                                                   \
    } while (false)
  #define vtWarnIfNot(cond,str)                                   \
    vtWarnIf(INVERT_COND(cond),str)
  #define vtWarnIfNotCode(code,cond,str)                          \
    vtWarnIfCode(code,INVERT_COND(cond),str)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H*/
