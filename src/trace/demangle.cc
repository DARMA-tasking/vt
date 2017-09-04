
#include "common.h"
#include "demangle.h"

namespace runtime { namespace trace {

/*static*/
ActiveFunctionDemangler::str_parsed_out_t
ActiveFunctionDemangler::parse_active_function_name(std::string const& str) {
  std::cout << "parse_active_function_name: str=" << str << std::endl;

  str_container_t const& elems = util_t::split_string(str, '&');
  assert(elems.size() == 2);
  auto const& last_elem = elems.at(1);

  std::regex fun_type("\\(([^(]*)", std::regex::extended);

  //std::cout << "searching str:" << last_elem << std::endl;
  std::smatch m;
  std::regex_search(last_elem, m, fun_type);

  std::string m0 = m[0];

  if (m0.substr(0, 5) == std::string("(void")) {
    //std::cout << "substr: \"" << m0.substr(0, 5) << "\"" << std::endl;
    m0 = m0.substr(6, m0.size());
  }

  //std::cout << "match: \"" << m0 << "\"" << std::endl;

  str_container_t const& namespace_no_temp = util_t::split_string(m0, '<');
  str_container_t const& namespace_elems = util_t::split_string(
    namespace_no_temp.at(0), ':'
  );

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
  std::stringstream clean_function_name_space;

  for (auto&& e : only_namespace_elems) {
    if (cur < len-2) {
      clean_namespace << e << "::";
    } else if (cur == len-2) {
      clean_namespace << e;
    } else if (cur == len-1) {
      clean_function_name_space << e;
    }
    cur++;
  }

  if (namespace_no_temp.size() > 1) {
    int cur = 0;
    for (auto&& x : namespace_no_temp) {
      if (cur != 0) {
        clean_function_name_space << "<" << x;
      }
      cur++;
    }
  }

  std::stringstream clean_function_name;
  clean_function_name << util_t::remove_spaces(clean_function_name_space.str());

  str_container_t const& args = util_t::split_string(last_elem, '(');

  assert(
    args.size() >= 3 and "args parsed must be at least 3"
  );

  auto args_only = args.at(2).substr(0, args.at(2).size() - 5);

  std::stringstream args_space;
  args_space << "(" << args_only << ")";

  std::stringstream clean_args;
  clean_args << util_t::remove_spaces(args_space.str());

  // std::cout << "\t namespace: \"" << clean_namespace.str() << "\"" << std::endl;
  // std::cout << "\t fun_name: \"" << clean_function_name.str() << "\"" << std::endl;
  // std::cout << "\t args: \"" << clean_args.str() << "\"" << std::endl;

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

/*static*/
ActiveFunctorDemangler::str_parsed_out_t
ActiveFunctorDemangler::parse_active_functor_name(std::string const& str) {
  return std::make_tuple("","");
}


}} //end namespace runtime::trace
