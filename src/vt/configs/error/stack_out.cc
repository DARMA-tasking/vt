/*
//@HEADER
// *****************************************************************************
//
//                                 stack_out.cc
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

#include "fmt/core.h"

#include "vt/configs/error/stack_out.h"

#include <cstdlib>
#include <vector>
#include <tuple>
#include <sstream>

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

namespace vt { namespace debug { namespace stack {

DumpStackType dumpStack(int skip) {
  void* callstack[128];
  int const max_frames = sizeof(callstack) / sizeof(callstack[0]);
  int num_frames = backtrace(callstack, max_frames);
  char** symbols = backtrace_symbols(callstack, num_frames);
  std::ostringstream trace_buf;
  StackVectorType tuple;

  for (auto i = skip; i < num_frames; i++) {
    //printf("%s\n", symbols[i]);

    std::string str = "";
    Dl_info info;
    if (dladdr(callstack[i], &info) && info.dli_sname) {
      char *demangled = nullptr;
      int status = -1;
      if (info.dli_sname[0] == '_') {
        demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
      }
      auto const call = status == 0 ?
        demangled : info.dli_sname == 0 ? symbols[i] : info.dli_sname;

      tuple.emplace_back(
        std::forward_as_tuple(
          static_cast<int>(2 + sizeof(void*) * 2), callstack[i], call,
          static_cast<char*>(callstack[i]) - static_cast<char*>(info.dli_saddr)
        )
      );

      auto const& t = tuple.back();
      str = fmt::format(
        "{:<4} {:<4} {:<15} {} + {}\n",
        i, std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t)
      );

      std::free(demangled);
    } else {

      tuple.emplace_back(
        std::forward_as_tuple(
          static_cast<int>(2 + sizeof(void*) * 2), callstack[i], symbols[i], 0
        )
      );

      auto const& t = tuple.back();
      str = fmt::format(
        "{:10} {} {} {}\n", i, std::get<0>(t), std::get<1>(t), std::get<2>(t)
      );
    }

    trace_buf << str;
  }
  std::free(symbols);

  if (num_frames == max_frames) {
    trace_buf << "[truncated]\n";
  }

  return std::make_tuple(trace_buf.str(),tuple);
}

}}} /* end namespace vt::debug::stack */

