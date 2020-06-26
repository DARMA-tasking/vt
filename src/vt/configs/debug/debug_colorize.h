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

#include <string>

namespace vt { namespace debug {

inline bool colorizeOutput() {
  return theArgConfig()->colorize_output;
}

inline std::string green()    { return colorizeOutput() ? "\033[32m"   : ""; }
inline std::string bold()     { return colorizeOutput() ? "\033[1m"    : ""; }
inline std::string magenta()  { return colorizeOutput() ? "\033[95m"   : ""; }
inline std::string red()      { return colorizeOutput() ? "\033[31m"   : ""; }
inline std::string bred()     { return colorizeOutput() ? "\033[31;1m" : ""; }
inline std::string reset()    { return colorizeOutput() ? "\033[00m"   : ""; }
inline std::string bd_green() { return colorizeOutput() ? "\033[32;1m" : ""; }
inline std::string it_green() { return colorizeOutput() ? "\033[32;3m" : ""; }
inline std::string un_green() { return colorizeOutput() ? "\033[32;4m" : ""; }
inline std::string byellow()  { return colorizeOutput() ? "\033[33;1m" : ""; }
inline std::string yellow()   { return colorizeOutput() ? "\033[33m"   : ""; }
inline std::string blue()     { return colorizeOutput() ? "\033[34m"   : ""; }

inline std::string emph(std::string str) {
  return magenta() + str + reset();
}
inline std::string reg(std::string str) {
  return green() + str + reset();
}
inline std::string vtPre() {
  return bd_green() + std::string("vt") + reset() + ": ";
}
inline std::string proc(vt::NodeType const& node)  {
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
