/*
//@HEADER
// *****************************************************************************
//
//                                  demangle.h
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

#if !defined INCLUDED_VT_UTILS_DEMANGLE_DEMANGLE_H
#define INCLUDED_VT_UTILS_DEMANGLE_DEMANGLE_H

#include "vt/config.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <assert.h>

namespace vt { namespace util { namespace demangle {

struct TemplateExtract {
  /// IFF tracing a tracing-enabled build, returns the compiler-dependent
  /// 'PRETTY PRINT' value of the ADORNED function as a string.
  /// The only useful bit here is the "[T = ..]" of the template in scope
  /// as the function name itself is self-evident; GCC and Clang supply this.
  /// This code will simply NOT COMPILE if the compiler is lacking required
  /// support; this method cannot be used if tracing is disabled.
  /// There should probably be a cmake check..
  template <class T>
  static constexpr char const* prettyFunctionForType() {
  #if vt_check_enabled(trace_enabled)
    return __PRETTY_FUNCTION__;
  #else
    assert(false && "Can only be used for a trace_enabled build.");
    return "";
  #endif
  }

  /// When the goal is to extract the value "as appearing"
  /// in the template parameterization, and not the type..
  template <class T, T PF_VALUE_NAME>
  static constexpr char const* prettyFunctionForValue() {
  #if vt_check_enabled(trace_enabled)
    return __PRETTY_FUNCTION__;
  #else
    assert(false && "Can only be used for a trace_enabled build.");
    return "";
  #endif
  }

  // T,T* 'overload' for GCC
  template <class T, T* PF_VALUE_NAME>
  static constexpr char const* prettyFunctionForValuePtr() {
  #if vt_check_enabled(trace_enabled)
    return __PRETTY_FUNCTION__;
  #else
    assert(false && "Can only be used for a trace_enabled build.");
    return "";
  #endif
  }

  /// Given a GCC/Clang-like __PRETTY_FUNCTION__ output,
  /// extract the template instantiation information as a string,
  /// assuming a SINGLE template parameter.
  /// Returns an empty string if such cannot be extracted.
  static std::string singlePfType(std::string const& pf);

  /// Given a GCG/Clang-long __PRETTY_FUNCTION__ output,
  /// extract the template information as a string,
  /// assuming tparam names the LAST template parameter.
  /// Returns an empty string if such cannot be extracted.
  static std::string lastNamedPfType(std::string const& spf, std::string const& tparam);

  /// Return the type of T, as a string.
  /// Requires compiler extension support.
  template <typename T>
  static std::string getTypeName() {
    return singlePfType(prettyFunctionForType<T>());
  }

  /// Return the type of T, as a string.
  /// Requires compiler extension support.
  /// (Might be able to use 'auto T' in C++17.)
  template <typename T, T value>
  static std::string getValueName() {
    return lastNamedPfType(prettyFunctionForValue<T,value>(), "PF_VALUE_NAME");
  }

  // T,T* 'overload' for GCC
  template <typename T, T* value>
  static std::string getValueNamePtr() {
    return lastNamedPfType(prettyFunctionForValuePtr<T,value>(), "PF_VALUE_NAME");
  }

  /// Given a string like 'a::b::c', return the namespace of 'a::b'.
  /// Does not strip out extra template parameterization artifacts.
  /// Removes any leading '&', if present (as in 'values representing types').
  static std::string getNamespace(std::string const& typestr);

  /// Given a string like 'a::b::c', return the barename of 'c'.
  /// Returns the bare name in absense of any namespace (eg. 'c' -> 'c').
  /// Removes any leading '&', if present (as in 'values representing types').
  static std::string getBarename(std::string const& typestr);

  /// Given a string like 'void (...)' (that is, the string representation of
  /// a function type.. return the argument section. As the name indicates
  /// this is somewhat limited.
  /// Returns the original if not starting with 'void (' or ending in ')'.
  static std::string getVoidFuncStrArgs(std::string const& typestr);
};

struct DemanglerUtils {
  static std::vector<std::string>
  splitString(std::string const& str, char delim);

  static std::string
  removeSpaces(std::string const& str);

  static std::string
  join(std::string const& delim, std::vector<std::string> const& strs);
};

}}} // end namespace vt::util::demangle

#endif /*INCLUDED_VT_UTILS_DEMANGLE_DEMANGLE_H*/
