
#include "common.h"
#include "demangle.h"

namespace runtime { namespace trace {

/*static*/
ActiveFunctionDemangler::str_parsed_out_t
ActiveFunctionDemangler::parse_active_function_name(std::string const& str) {
  str_container_t const& elems = util_t::split_string(str, '&');
  assert(elems.size() == 2);
  auto const& last_elem = elems.at(1);

  std::regex fun_type("\\(([a-zA-Z0-9_:]*)", std::regex::extended);

  //std::cout << "searching str:" << last_elem << std::endl;
  std::smatch m;
  std::regex_search(last_elem, m, fun_type);
  //std::cout << "match: \"" << m[0] << "\"" << std::endl;

  str_container_t const& namespace_elems = util_t::split_string(m[0], ':');

  str_container_t only_namespace_elems;

  for (auto&& e : namespace_elems) {
    if (e != "") {
      if (e[0] != '(') {
        only_namespace_elems.push_back(e);
      } else {
        str_container_t const& non_paren = util_t::split_string(e, '(');
        only_namespace_elems.push_back(non_paren.at(1));
      }
    }
  }

  int cur = 0, len = only_namespace_elems.size();
  std::stringstream clean_namespace;
  std::stringstream clean_function_name;

  for (auto&& e : only_namespace_elems) {
    if (cur < len-2) {
      clean_namespace << e << "::";
    } else if (cur == len-2) {
      clean_namespace << e;
    } else if (cur == len-1) {
      clean_function_name << e;
    }
    cur++;
  }

  str_container_t const& args = util_t::split_string(last_elem, '(');

  assert(
    args.size() >= 3 and "args parsed must be at least 3"
  );

  str_container_t const& args_clean = util_t::split_string(args.at(2), '>');

  assert(
    args.size() >= 0 and "args parsed must be at least 0"
  );

  str_container_t const& args_clean_no_paran = util_t::split_string(args_clean.at(0), ')');

  std::stringstream clean_args;
  clean_args << "(" << args_clean_no_paran.at(0) << ")";

  // std::cout << "namespace: \"" << clean_namespace.str() << "\"" << std::endl;
  // std::cout << "fun_name: \"" << clean_function_name.str() << "\"" << std::endl;
  // std::cout << "args: \"" << clean_args.str() << "\"" << std::endl;

  #if backend_check_enabled(trace_enabled) && backend_check_enabled(trace)
  std::cout << "Parsed Active Fn: " << "\t"
            << clean_namespace.str() << " :: "
            << clean_function_name.str()
            << clean_args.str() << std::endl;
  #endif

  std::stringstream combined_fun_name;
  combined_fun_name << clean_function_name.str() << clean_args.str();

  return std::make_tuple(clean_namespace.str(), combined_fun_name.str());
}

}} //end namespace runtime::trace
