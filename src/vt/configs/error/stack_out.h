
#if !defined INCLUDED_VT_CONFIGS_ERROR_STACK_OUT_H
#define INCLUDED_VT_CONFIGS_ERROR_STACK_OUT_H

#include "vt/config.h"

#include <cstdlib>
#include <vector>
#include <tuple>
#include <sstream>

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

namespace vt { namespace debug { namespace stack {

using StackTupleType  = std::tuple<int32_t,void*,std::string,std::size_t>;
using StackVectorType = std::vector<StackTupleType>;
using DumpStackType   = std::tuple<std::string,StackVectorType>;

/*
 * This function automatically produce a backtrace of the stack with demangled
 * function names and method name.
 */
inline DumpStackType dumpStack(int skip = 0) {
  void* callstack[128];
  char buf[1024];
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

#endif /*INCLUDED_VT_CONFIGS_ERROR_STACK_OUT_H*/
