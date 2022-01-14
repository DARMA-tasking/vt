/*
//@HEADER
// *****************************************************************************
//
//                                 stack_out.cc
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

#include "vt/configs/error/stack_out.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/context/context.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>

namespace vt { namespace debug { namespace stack {

DumpStackType dumpStack(int skip) {
  StackVectorType stack;

  unw_cursor_t cursor;
  unw_context_t context;

  // Initialize cursor to current frame for local unwinding.
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  // Unwind frames one by one, going up the frame stack.
  int i = 0;
  do {
    if (i++ < skip) {
      continue;
    }

    unw_word_t offset, pc;
    unw_get_reg(&cursor, UNW_REG_IP, &pc);
    if (pc == 0) {
      break;
    }

    char sym[256];
    if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
      char* nameptr = sym;
      int status;
      char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
      if (status == 0) {
        nameptr = demangled;
      }

      stack.emplace_back(
        std::forward_as_tuple(
          static_cast<int>(2 + sizeof(void*) * 2), pc, nameptr, offset)
      );

      std::free(demangled);
    } else {
      // FIXME!
      std::printf(" -- error: unable to obtain symbol name for this frame\n");
    }
  }
  while (unw_step(&cursor) > 0);

  // FIXME! - is "truncated" backup still necessary?
  return std::make_tuple("",stack);
}

std::string prettyPrintStack(StackVectorType const& stack) {
  auto green      = ::vt::debug::green();
  auto bred       = ::vt::debug::bred();
  auto reset      = ::vt::debug::reset();
  auto magenta    = ::vt::debug::magenta();
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
      "{}{}{:<3}{} {}{:<3} {:#x}{} {}{}{} + {}{}\n",
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
