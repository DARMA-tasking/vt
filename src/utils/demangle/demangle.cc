

#include "config.h"
#include "utils/demangle/demangle.h"

#include <vector>
#include <string>
#include <list>
#include <cstring>

namespace vt { namespace util { namespace demangle {

/*static*/ ActiveFunctionDemangler::StrParsedOutType
ActiveFunctionDemangler::parseActiveFunctionName(std::string const& str) {
  using CountType = int32_t;
  using CharType = char;

  std::string clean_namespace = {};
  std::string clean_funcname  = {};
  std::string clean_params    = {};

  ::fmt::print("ADAPT before: adapt={}\n",str);

  std::string const adapt_start = "FunctorAdapter";

  CountType start = 0;
  for (CountType i = 0; i < str.size() - adapt_start.size() - 1; i++) {
    auto const substr = str.substr(i, adapt_start.size());
    //::fmt::print("ADAPT XX substr: substr={}\n",substr);
    if (substr == adapt_start) {
      //::fmt::print("ADAPT substr: substr={}\n",substr);
      start = i;
      break;
    }
  }

  CountType   const  str_offset_right_ns = adapt_start.size() + 1;
  CountType   const  str_offset_right_tn = start;
  std::string const  func_adapt_params   = str.substr(
                     str_offset_right_ns + str_offset_right_tn,
    str.size() - 2 - str_offset_right_ns - str_offset_right_tn
  );

  ::fmt::print("ADAPT: adapt={}\n",func_adapt_params);

  CountType      paren_count  [2]  = {0,0};
  CountType      bracket_count[2]  = {0,0};
  CountType      brace_count  [2]  = {0,0};
  CountType      caret_count  [2]  = {0,0};
  CharType const paren        [2]  = {'(', ')'};
  CharType const bracket      [2]  = {'[', ']'};
  CharType const brace        [2]  = {'{', '}'};
  CharType const caret        [2]  = {'<', '>'};
  CharType const delim      = ',';

  std::list<std::tuple<CharType const*, CountType*>> tuples = {
    {paren   , paren_count   },
    {bracket , bracket_count },
    {brace   , brace_count   },
    {caret   , caret_count   }
  };
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
    if (ch == delim && equal) {
      cur_param++;
      pieces.resize(cur_param + 1);
    } else {
      pieces.at(cur_param) += ch;
    }
  }

  for (auto&& elm : pieces) {
    ::fmt::print("ADAPT: split: adapt={}\n",elm);
  }

  assert(pieces.size() == 2 && "Must be two pieces");

  auto const func_args = pieces[0];
  auto const func_name = pieces[1].substr(2,pieces[1].size()-2);

  ::fmt::print("ADAPT: func_args: adapt={}\n",func_args);
  ::fmt::print("ADAPT: func_name: adapt={}\n",func_name);

  std::string const delim_str = {"::"};
  CountType cur_func_piece = 0;
  std::vector<std::string> func_name_pieces(cur_func_piece + 1);
  for (int i = 0; i < func_name.size(); i++) {
    if (
      func_name.size() - 1   >    i + 1           &&
      func_name[i    ]       ==   delim_str[0]    &&
      func_name[i + 1]       ==   delim_str[1]
    ) {
      i++;
      cur_func_piece++;
      func_name_pieces.resize(cur_func_piece + 1);
    } else {
      func_name_pieces[cur_func_piece] += func_name[i];
    }
  }

  for (auto&& elm : func_name_pieces) {
    ::fmt::print("ADAPT: func_name piece: adapt={}\n",elm);
  }

  std::string fn_name = {};
  std::string ns_fused = {"::"};
  std::vector<std::string> func_name_ns(func_name_pieces.size() - 1);
  for (auto iter = func_name_pieces.begin(); iter != func_name_pieces.end() - 1; ++iter) {
    ::fmt::print("ADAPT: NS piece: adapt={}\n",*iter);
    ns_fused += *iter + "::";
  }

  clean_funcname = *(func_name_pieces.end() - 1);
  clean_namespace = ns_fused;

  CountType const init_offset = 6;
  auto const init_sub = func_args.substr(0,init_offset);
  if (init_sub == "void (") {
    auto const args = func_args.substr(
                          init_offset,
      func_args.size() - (init_offset) - 1
    );
    ::fmt::print("ADAPT: args={}\n", args);
    clean_params = DemanglerUtils::removeSpaces(args);
  } else {
    clean_params = "";
  }

  ::fmt::print(
    "ADAPT: \n"
    "\t CLEAN namespace = \"{}\" \n"
    "\t CLEAN  funcname = \"{}\" \n"
    "\t CLEAN  params   = \"{}\" \n""\n",
    clean_namespace, clean_funcname, clean_params
  );

  auto const demangled = DemangledName{
    clean_namespace, clean_funcname, clean_params
  };
  return demangled;
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

}}} // end namespace vt::util::demangle
