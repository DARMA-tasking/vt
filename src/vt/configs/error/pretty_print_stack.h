/*
//@HEADER
// *****************************************************************************
//
//                             pretty_print_stack.h
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

#if !defined INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_STACK_H
#define INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_STACK_H

#include "vt/config.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/debug/debug_colorize.h"

#include <vector>
#include <tuple>
#include <string>

namespace vt { namespace debug { namespace stack {

inline std::string prettyPrintStack(StackVectorType const& stack) {
  auto green      = ::vt::debug::green();
  auto red        = ::vt::debug::red();
  auto bred       = ::vt::debug::bred();
  auto reset      = ::vt::debug::reset();
  auto bd_green   = ::vt::debug::bd_green();
  auto magenta    = ::vt::debug::magenta();
  auto blue       = ::vt::debug::blue();
  auto yellow     = ::vt::debug::yellow();
  auto vt_pre     = ::vt::debug::vtPre();
  auto node       = ::vt::theContext()->getNode();
  auto node_str   = ::vt::debug::proc(node);
  auto prefix     = vt_pre + node_str + " ";
  auto seperator  = fmt::format("{}{}{:-^120}{}\n", prefix, yellow, "", reset);
  auto title_node = fmt::format("on Node {}", node);
  auto title      = fmt::format(" Dump Stack Backtrace {} ", title_node);

  std::string out = "";

  out += fmt::format("{}", seperator);
  out += fmt::format("{}{}{:-^120}{}\n", prefix, yellow, title, reset);
  out += fmt::format("{}", seperator);

  int i = 0;
  for (auto&& t : stack) {
    auto ret_str = fmt::format(
      "{}{}{:<3}{} {}{:<3} {:<13}{} {}{}{} + {}{}\n",
      prefix,
      bred, i, reset,
      magenta, std::get<0>(t), std::get<1>(t), reset,
      green,   std::get<2>(t), reset,
      std::get<3>(t), reset
    );
    out += ret_str;
    i++;
  }

  //out += seperator + seperator + seperator;

  return out;
}

}}} /* end namespace vt::debug::stack */

#endif /*INCLUDED_VT_CONFIGS_ERROR_PRETTY_PRINT_STACK_H*/
