
#if ! defined __RUNTIME_TRANSPORT_TRACE_DEMANGLE__
#define __RUNTIME_TRANSPORT_TRACE_DEMANGLE__

#include "common.h"

#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <cxxabi.h>

namespace runtime { namespace trace {

using str_container_t = std::vector<std::string>;

struct DemanglerUtils {
  template <class T>
  static inline std::string
  get_type_name() {
    return typeid(T).name();
  }

  static inline std::string
  demangle(std::string const& name) {
    int status = -1;
    char* result = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
    return status == 0 ? std::string(result) : name;
  }

  template <typename T>
  static inline std::string
  get_demangled_type() {
    auto const& type = get_type_name<T>();
    return demangle(type);
  }

  template <typename StringOut>
  static inline void
  split_string(std::string const& s, char delim, StringOut result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
      *(result++) = item;
    }
  }

  static inline str_container_t
  split_string(std::string const& str, char delim) {
    str_container_t elems;
    split_string(str, delim, std::back_inserter(elems));
    return elems;
  }

  static inline std::string
  remove_spaces(std::string const& str) {
    str_container_t const& str_split = split_string(str, ' ');
    std::stringstream clean;
    for (auto&& x : str_split) {
      clean << x;
    }
    return clean.str();
  }
};

/*
 *                   Example Format for active message function:
 *
 *  runtime::auto_registry::Runnable<
 *    runtime::auto_registry::FunctorAdapter<
 *      void (runtime::term::TermCounterMsg*),
 *      &(runtime::term::TerminationDetector::propagate_epoch_handler(
 *        runtime::term::TermCounterMsg*)
 *       )
 *    >
 *  >
 */

struct ActiveFunctionDemangler {
  using str_parsed_out_t = std::tuple<std::string, std::string>;
  using util_t = DemanglerUtils;

  static str_parsed_out_t
  parse_active_function_name(std::string const& str);
};

struct ActiveFunctorDemangler {
  using str_parsed_out_t = std::tuple<std::string, std::string>;
  using util_t = DemanglerUtils;

  static str_parsed_out_t
  parse_active_functor_name(std::string const& name, std::string const& args);
};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_DEMANGLE__*/
