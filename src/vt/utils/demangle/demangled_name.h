/*
//@HEADER
// *****************************************************************************
//
//                               demangled_name.h
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

#if !defined INCLUDED_UTILS_DEMANGLE_DEMANGLED_NAME_H
#define INCLUDED_UTILS_DEMANGLE_DEMANGLED_NAME_H

#include "vt/config.h"

#include <string>

namespace vt { namespace util { namespace demangle {

struct DemangledName {
  DemangledName(
    std::string const& in_ns, std::string const& in_func,
    std::string const& in_args
  ) : namespace_(in_ns), funcname_(in_func), params_(in_args),
      func_with_params_(in_func + "(" + in_args + ")")
  { }

  std::string getNamespace()  const { return namespace_       ; }
  std::string getFunc()       const { return funcname_        ; }
  std::string getParams()     const { return params_          ; }
  std::string getFuncParams() const { return func_with_params_; }

private:
  std::string const namespace_        = {};
  std::string const funcname_         = {};
  std::string const params_           = {};
  std::string const func_with_params_ = {};
};

}}} /* end namespace vt::util::demangle */

#endif /*INCLUDED_UTILS_DEMANGLE_DEMANGLED_NAME_H*/
