/*
//@HEADER
// ************************************************************************
//
//                          demangle.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/


#include "vt/config.h"
#include "vt/utils/demangle/demangle.h"
#include "vt/context/context.h"

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

  debug_print(
    verbose, gen, node,
    "ADAPT before: adapt={}\n", str
  );

  std::string const adapt_start     = "FunctorAdapter";
  std::string const adapt_start_mem = "FunctorAdapterMember";

  CountType const start_sentinel = -1;
  CountType start                = start_sentinel;

  bool found_member = false;
  for (CountType i = 0; i < str.size() - adapt_start_mem.size() - 1; i++) {
    auto const substr = str.substr(i, adapt_start_mem.size());
    debug_print(
      verbose, gen, node,
      "ADAPT XX substr: substr={}\n", substr
    );
    if (substr == adapt_start_mem) {
      debug_print(
        verbose, gen, node,
        "ADAPT substr: substr={}\n", substr
      );
      start = i;
      found_member = true;
      break;
    }
  }

  if (not found_member) {
    for (CountType i = 0; i < str.size() - adapt_start.size() - 1; i++) {
      auto const substr = str.substr(i, adapt_start.size());
      debug_print(
        verbose, gen, node,
        "ADAPT XX substr: substr={}\n", substr
      );
      if (substr == adapt_start) {
        debug_print(
          verbose, gen, node,
          "ADAPT substr: substr={}\n", substr
        );
        start = i;
        break;
      }
    }
  }

  vtAssert(start != start_sentinel, "String must be found");

  std::string adapt = found_member ? adapt_start_mem : adapt_start;

  std::string func_adapt_params;

# if defined(__clang__)
  CountType   const  str_offset_right_ns = adapt.size() + 1;
  CountType   const  str_offset_right_tn = start;

  func_adapt_params  = str.substr(
                     str_offset_right_ns + str_offset_right_tn,
    str.size() - 2 - str_offset_right_ns - str_offset_right_tn
  );
# elif defined(__GNUC__)
  CountType   const  str_offset_right_ns = adapt.size() + 1;
  CountType   const  str_offset_right_tn = start;

  func_adapt_params  = str.substr(
                     str_offset_right_ns + str_offset_right_tn,
    str.size() - 1 - str_offset_right_ns - str_offset_right_tn
  );
  auto const split_semi = DemanglerUtils::splitString(func_adapt_params,';');
  vtAssert(split_semi.size() > 0, "Must have at least one element");
  func_adapt_params = split_semi[0].substr(0,split_semi[0].size()-1);
#else
  // @todo: what should we do here
  func_adapt_params = str;
# endif

  debug_print(
    verbose, gen, node,
    "ADAPT: adapt={}\n", func_adapt_params
  );

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
    std::make_tuple(paren   , paren_count   ),
    std::make_tuple(bracket , bracket_count ),
    std::make_tuple(brace   , brace_count   ),
    std::make_tuple(caret   , caret_count   )
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
    debug_print(
      verbose, gen, node,
      "ADAPT: split: adapt={}\n", elm
    );
  }

  vtAssertInfo(
    pieces.size() == 2, "Must be two pieces", pieces[0]
  );

  auto const func_args = pieces[0];

#   if defined(__clang__)
    auto const func_name = pieces[1].substr(2,pieces[1].size()-2);
#   elif defined(__GNUC__)
    auto       func_name = pieces[1].substr(1,pieces[1].size()-1);
#   else
    auto const func_name = pieces[1];
#   endif

  debug_print(
    verbose, gen, node,
    "ADAPT: func_args: adapt={}\n", func_args
  );
  debug_print(
    verbose, gen, node,
    "ADAPT: func_name: adapt={}\n", func_name
  );

# if defined(__clang__)
# elif defined(__GNUC__)
  std::string func_name_no_template = {};
  CountType temp_in = 0, temp_out = 0;
  for (int i = 0; i < func_name.size(); i++) {
    if (func_name[i] == '<') { temp_in++;  };
    if (func_name[i] == '>') { temp_out++; };
    if (temp_in == temp_out && func_name[i] != '>') {
      func_name_no_template += func_name[i];
    }
  }
  func_name = DemanglerUtils::removeSpaces(func_name_no_template);
  debug_print(
    verbose, gen, node,
    "ADAPT: func_name_no_template: adapt={}\n", func_name_no_template
  );
# endif

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
      func_name_pieces.at(cur_func_piece) += func_name[i];
    }
  }

  #if backend_check_enabled(verbose) && backend_check_enabled(gen)
    for (auto&& elm : func_name_pieces) {
      debug_print(
        verbose, gen, node,
        "ADAPT: func_name piece: adapt={}\n", elm
      );
    }
  #endif

  std::string fused_namespace = {};
  if (func_name_pieces.size() < 2) {
    // There is no namespace (in global); use "::" to represent it
    fused_namespace = "::";
  } else {
    for (auto iter = func_name_pieces.begin(); iter != func_name_pieces.end() - 1; ++iter) {
      debug_print(
        verbose, gen, node,
        "ADAPT: NS piece: adapt={}\n", *iter
      );
      fused_namespace += *iter + "::";
    }
  }

  clean_funcname = *(func_name_pieces.end() - 1);
  clean_namespace = fused_namespace;

# if defined(__clang__)
  CountType const init_offset = 6;
# elif defined(__GNUC__)
  CountType const init_offset = 5;
# else
  CountType const init_offset = 4;
# endif

  auto const init_sub = func_args.substr(0,init_offset);

# if defined(__clang__)
  std::string const leading_void_str = "void (";
# elif defined(__GNUC__)
  std::string const leading_void_str = "void(";
# else
  std::string const leading_void_str = "void";
# endif

  if (init_sub == leading_void_str) {
    auto const args = func_args.substr(
                          init_offset,
      func_args.size() - (init_offset) - 1
    );
    debug_print(
      verbose, gen, node,
      "ADAPT: args={}\n", args
    );
    clean_params = DemanglerUtils::removeSpaces(args);
  } else {
    clean_params = "";
  }

  if (found_member) {
    CountType pstart = 0;
    for (auto i = 0; i < clean_params.size(); i++) {
      if (clean_params[i] == '*') {
        pstart = i+3;
        break;
      }
    }
    clean_params = clean_params.substr(pstart,clean_params.size());
  }

  debug_print(
    verbose, gen, node,
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
  debug_print(
    verbose, gen, node,
    "parseActiveFunctorName: \n"
    "\t input name = \"{}\" \n"
    "\t input args = \"{}\" \n",
    name, args
  );

  auto const ret = ActiveFunctionDemangler::parseActiveFunctionName(name);

  debug_print(
    verbose, gen, node,
    "parseActiveFunctorName: \n"
    "\t CLEAN namespace = \"{}\" \n"
    "\t CLEAN funcname = \"{}\" \n",
    ret.getNamespace(),
    ret.getFunc()
  );

  return ret;
}

}}} // end namespace vt::util::demangle
