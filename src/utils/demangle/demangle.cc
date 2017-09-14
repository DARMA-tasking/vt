
#include "config.h"
#include "demangle.h"

namespace vt { namespace demangle {

/*static*/ ActiveFunctionDemangler::StrParsedOutType
ActiveFunctionDemangler::parseActiveFunctionName(std::string const& str) {
  //std::cout << "parse_active_function_name: str=" << str << std::endl;

  StrContainerType const& elems = UtilType::splitString(str, '&');
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

  StrContainerType const& namespace_no_temp = UtilType::splitString(m0, '<');
  StrContainerType const& namespace_elems = UtilType::splitString(
    namespace_no_temp.at(0), ':'
  );

  StrContainerType only_namespace_elems;

  for (auto&& e : namespace_elems) {
    if (e != "") {
      if (e[0] != '(') {
        only_namespace_elems.push_back(e);
      } else {
        StrContainerType const& non_paren = UtilType::splitString(e, '(');
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
  clean_function_name << UtilType::removeSpaces(clean_function_name_space.str());

  StrContainerType const& args = UtilType::splitString(last_elem, '(');

  assert(
    args.size() >= 3 and "args parsed must be at least 3"
  );

  auto args_only = args.at(2).substr(0, args.at(2).size() - 5);

  std::stringstream args_space;
  args_space << "(" << args_only << ")";

  std::stringstream clean_args;
  clean_args << UtilType::removeSpaces(args_space.str());

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

/*static*/ ActiveFunctorDemangler::StrParsedOutType
ActiveFunctorDemangler::parseActiveFunctorName(
  std::string const& name, std::string const& args
) {
  // std::cout << "parse_active_functor_name:"
  //           << "name=" << name << ", "
  //           << "args=" << args
  //           << std::endl;

  auto args_clean = args.substr(29, args.size()-30);

  std::stringstream args_ret;
  args_ret << "operator()(";
  args_ret << args_clean;
  args_ret << ")";

  std::stringstream args_no_space;
  args_no_space << UtilType::removeSpaces(args_ret.str());

  return std::make_tuple(name,args_no_space.str());
}

}} //end namespace vt::demangle
