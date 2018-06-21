

#include "config.h"
#include "utils/demangle/demangle.h"

#include <vector>
#include <string>

namespace vt { namespace demangle {

/*static*/ ActiveFunctionDemangler::StrParsedOutType
ActiveFunctionDemangler::parseActiveFunctionName(std::string const& str) {
  //std::cout << "parse_active_function_name: str=" << str << std::endl;

  ::fmt::print("ADAPT before: adapt={}\n\n",str);

  auto func_adapt_params = str.substr(34+28,str.size()-2-34-28);

  ::fmt::print("ADAPT: adapt={}\n\n",func_adapt_params);

  using CountType = int32_t;
  using CharType = char;
  CharType const delim = ',';
  CountType paren_count  [2]  = {0,0};
  CountType bracket_count[2]  = {0,0};
  CountType brace_count  [2]  = {0,0};
  CountType caret_count  [2]  = {0,0};
  // CountType paren_open   = 0,   paren_close   = 0;
  // CountType bracket_open = 0,   bracket_close = 0;
  // CountType brace_open   = 0,   brace_close   = 0;
  // CountType caret_open   = 0,   caret_close   = 0;
  CharType const paren  [2] = {'(', ')'};
  CharType const bracket[2] = {'[', ']'};
  CharType const brace  [2] = {'{', '}'};
  CharType const caret  [2] = {'<', '>'};

  std::vector<std::tuple<CharType const*, CountType*>> tuples = {
    {paren   , paren_count   },
    {bracket , bracket_count },
    {brace   , brace_count   },
    {caret   , caret_count   }
  };
  //make_tuple(paren, paren_count)
  // char const open_paren   = '(',  close_paren   = ')';
  // char const open_bracket = '[',  close_bracket = ']';
  // char const open_brace   = '{',  close_brace   = '}';
  // char const open_caret   = '<',  close_caret   = '>';
  CountType cur_param    = 0;
  std::vector<std::string> pieces(cur_param + 1);
  for (auto&& ch : func_adapt_params) {
    for (auto const& tup : tuples) {
      if (ch == std::get<0>(tup)[0]) { std::get<1>(tup)[0]++; }
      if (ch == std::get<0>(tup)[1]) { std::get<1>(tup)[1]++; }
    }
    bool equal = true;
    for (auto const& tup : tuples) {
      if (std::get<1>(tup)[0] != std::get<1>(tup)[1]) {
        equal = false;
        break;
      }
    }
    // if (ch == paren[0]) { paren_open   ++; }
    // if (ch == paren[1]) { paren_close  ++; }
    // if (ch == '[') { bracket_open ++; }
    // if (ch == ']') { bracket_close++; }
    // if (ch == '}') { brace_open   ++; }
    // if (ch == '{') { brace_close  ++; }
    // if (ch == '<') { caret_open   ++; }
    // if (ch == '>') { caret_close  ++; }
    // if (ch == '(') { paren_open   ++; }
    // if (ch == ')') { paren_close  ++; }
    // if (ch == '[') { bracket_open ++; }
    // if (ch == ']') { bracket_close++; }
    // if (ch == '}') { brace_open   ++; }
    // if (ch == '{') { brace_close  ++; }
    // if (ch == '<') { caret_open   ++; }
    // if (ch == '>') { caret_close  ++; }
    if (ch == delim && equal) {
      cur_param++;
      pieces.resize(cur_param + 1);
    } else {
      pieces.at(cur_param) += ch;
    }
    // if (
    //   ch == delim                   &&
    //   paren_open == paren_close     &&
    //   bracket_open == bracket_close &&
    //   brace_open == brace_close     &&
    //   caret_open == caret_close
    // ) {
    //   cur_param++;
    //   pieces.resize(cur_param + 1);
    // } else {
    //   pieces.at(cur_param) += ch;
    // }
  }

  for (auto&& elm : pieces) {
    ::fmt::print("ADAPT: split: adapt={}\n\n",elm);
  }

  assert(pieces.size() == 2 && "Must be two pieces");

  auto func_args = pieces[0];
  auto func_name = pieces[1].substr(2,pieces[1].size()-2);

  ::fmt::print("ADAPT: func_args: adapt={}\n\n",func_args);
  ::fmt::print("ADAPT: func_name: adapt={}\n\n",func_name);

  char const delim1 = ':';
  char const delim2 = ':';
  int cur_func_piece = 0;
  std::vector<std::string> func_name_pieces(cur_func_piece + 1);
  for (int i = 0; i < func_name.size(); i++) {
    if (
      func_name.size() - 1   >    i + 1     &&
      func_name[i    ]       ==   delim1    &&
      func_name[i + 1]       ==   delim2
    ) {
      i++;
      cur_func_piece++;
      func_name_pieces.resize(cur_func_piece + 1);
    } else {
      func_name_pieces[cur_func_piece] += func_name[i];
    }
  }

  for (auto&& elm : func_name_pieces) {
    ::fmt::print("ADAPT: func_name piece: adapt={}\n\n",elm);
  }

  // auto pre_elms = UtilType::splitString(str, '(');
  // assert(pre_elms.size() == 2);
  // auto ns = pre_elms.at(0);

  // StrContainerType const& elems = UtilType::splitString(str, '&');
  // assert(elems.size() == 2);
  // auto const& last_elem = elems.at(1);

  // std::regex fun_type("\\(([^(]*)", std::regex::extended);

  // //std::cout << "searching str:" << last_elem << std::endl;
  // std::smatch m;
  // std::regex_search(last_elem, m, fun_type);

  // std::string m0 = m[0];

  // if (m0.substr(0, 5) == std::string("(void")) {
  //   //std::cout << "substr: \"" << m0.substr(0, 5) << "\"" << std::endl;
  //   m0 = m0.substr(6, m0.size());
  // }

  // //std::cout << "match: \"" << m0 << "\"" << std::endl;

  // StrContainerType const& namespace_no_temp = UtilType::splitString(m0, '<');
  // StrContainerType const& namespace_elems = UtilType::splitString(
  //     namespace_no_temp.at(0), ':'
  // );

  // StrContainerType only_namespace_elems;

  // for (auto                      && e : namespace_elems) {
  //   if (e != "") {
  //     if (e[0] != '(') {
  //       only_namespace_elems.push_back(e);
  //     } else {
  //       StrContainerType const& non_paren = UtilType::splitString(e, '(');
  //       only_namespace_elems.push_back(non_paren.at(1));
  //     }
  //   }
  // }

  // int cur = 0, len = only_namespace_elems.size();
  // std::stringstream clean_namespace;
  // std::stringstream clean_function_name_space;

  // for (auto                      && e : only_namespace_elems) {
  //   if (cur < len - 2) {
  //     clean_namespace << e << "::";
  //   } else if (cur == len - 2) {
  //     clean_namespace << e;
  //   } else if (cur == len - 1) {
  //     clean_function_name_space << e;
  //   }
  //   cur++;
  // }

  // if (namespace_no_temp.size() > 1) {
  //   int cur = 0;
  //   for (auto&& x : namespace_no_temp) {
  //     if (cur != 0) {
  //       clean_function_name_space << "<" << x;
  //     }
  //     cur++;
  //   }
  // }

  // std::stringstream clean_function_name;
  // clean_function_name
  //     << UtilType::removeSpaces(clean_function_name_space.str());

  // StrContainerType const& args = UtilType::splitString(last_elem, '(');

  // assert(
  //     args.size() >= 3 and "args parsed must be at least 3"
  // );

  // auto args_only = args.at(2).substr(0, args.at(2).size() - 5);

  // std::stringstream args_space;
  // args_space << "(" << args_only << ")";

  // std::stringstream clean_args;
  // clean_args << UtilType::removeSpaces(args_space.str());

  // // std::cout << "\t namespace: \"" << clean_namespace.str() << "\"" << std::endl;
  // // std::cout << "\t fun_name: \"" << clean_function_name.str() << "\"" << std::endl;
  // // std::cout << "\t args: \"" << clean_args.str() << "\"" << std::endl;

  // #if backend_check_enabled(trace_enabled) && backend_check_enabled(trace)
  // std::cout << "Parsed Active Fn: " << "\t"
  //           << clean_namespace.str() << " :: "
  //           << clean_function_name.str()
  //           << clean_args.str() << std::endl;
  // #endif

  // std::stringstream combined_fun_name;
  // combined_fun_name << clean_function_name.str() << clean_args.str();

  // int status = 0;
  // auto res = abi::__cxa_demangle(str.c_str(), nullptr, 0, &status);

  // ::fmt::print("status={}\n",status);
  // ::fmt::print("status={},res={}\n",status,res);

  // return std::make_tuple(clean_namespace.str(), std::string{res});
  return std::make_tuple(std::string{}, std::string{});
}

/*static*/ ActiveFunctorDemangler::StrParsedOutType
ActiveFunctorDemangler::parseActiveFunctorName(
    std::string const& name, std::string const& args
) {
  // std::cout << "parse_active_functor_name:"
  //           << "name=" << name << ", "
  //           << "args=" << args
  //           << std::endl;

  auto args_clean = args.substr(29, args.size() - 30);

  std::stringstream args_ret;
  args_ret << "operator()(";
  args_ret << args_clean;
  args_ret << ")";

  std::stringstream args_no_space;
  args_no_space << UtilType::removeSpaces(args_ret.str());

  return std::make_tuple(name, args_no_space.str());
}

}}  // end namespace vt::demangle
