/*
//@HEADER
// *****************************************************************************
//
//                               debug_colorize.h
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

#if !defined INCLUDED_VT_CONFIGS_DEBUG_DEBUG_COLORIZE_H
#define INCLUDED_VT_CONFIGS_DEBUG_DEBUG_COLORIZE_H

#include "vt/configs/arguments/args.h"
#include "vt/configs/types/types_type.h"

#include <string>
#include <unistd.h>
// #include <sys/stat.h>

namespace vt { namespace debug {

inline auto istty() -> bool {
  return isatty(fileno(stdout)) ? true : false;
}

inline auto ttyc() -> bool {
  auto nocolor = arguments::ArgConfig::vt_no_color ? false : true;
  auto tty = arguments::ArgConfig::vt_auto_color ? istty() : nocolor;
  return tty;
}

inline auto green()    -> std::string { return ttyc() ? "\033[32m"   : ""; }
inline auto bold()     -> std::string { return ttyc() ? "\033[1m"   : ""; }
inline auto magenta()  -> std::string { return ttyc() ? "\033[95m"   : ""; }
inline auto red()      -> std::string { return ttyc() ? "\033[31m"  : ""; }
inline auto bred()     -> std::string { return ttyc() ? "\033[31;1m" : ""; }
inline auto reset()    -> std::string { return ttyc() ? "\033[00m"   : ""; }
inline auto bd_green() -> std::string { return ttyc() ? "\033[32;1m" : ""; }
inline auto it_green() -> std::string { return ttyc() ? "\033[32;3m" : ""; }
inline auto un_green() -> std::string { return ttyc() ? "\033[32;4m" : ""; }
inline auto byellow()  -> std::string { return ttyc() ? "\033[33;1m" : ""; }
inline auto yellow()   -> std::string { return ttyc() ? "\033[33m"   : ""; }
inline auto blue()     -> std::string { return ttyc() ? "\033[34m"   : ""; }

inline auto emph(std::string str) -> std::string  {
  return magenta() + str + reset();
}
inline auto reg(std::string str) -> std::string  {
  return green() + str + reset();
}
inline auto vtPre() -> std::string  {
  return bd_green() + std::string("vt") + reset() + ": ";
}
inline auto proc(vt::NodeType const& node) -> std::string  {
  return blue() + "[" + std::to_string(node) + "]" + reset();
}

// static bool ttyi(FILE* stream) {
//   struct stat stream_stat;
//   if (fstat(fileno(stream), &stream_stat) == 0) {
//     if (stream_stat.st_mode & S_IFREG) {
//       return true;
//     }
//   }
//   return false;
// }

}} /* end namespace vt::debug */


#endif /*INCLUDED_VT_CONFIGS_DEBUG_DEBUG_COLORIZE_H*/
