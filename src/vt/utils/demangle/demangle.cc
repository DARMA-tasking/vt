/*
//@HEADER
// *****************************************************************************
//
//                                 demangle.cc
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

#include "vt/utils/demangle/demangle.h"

#include <vector>
#include <string>
#include <list>
#include <cstring>

#include <sstream>
#include <iterator>
#include <algorithm>

#include <iostream>

namespace vt { namespace util { namespace demangle {

//
// TemplateExtract
//

/*static*/ std::string
TemplateExtract::singlePfType(std::string const& pf) {
  // PF -> .. [T = (...)]
  return lastNamedPfType(pf, "");
}

/*static*/ std::string
TemplateExtract::lastNamedPfType(std::string const& pf, std::string const& tparam) {
  // PF -> .. [T1 = .., T2 = .., TPARAM = (...)]
  std::string seek = tparam + " = ";

  size_t i = pf.find(seek);
  if (i == std::string::npos)
    return "";
  i = i + seek.length();

  return pf.substr(i, pf.length() - i - 1);
}

/*static*/ std::string
TemplateExtract::getNamespace(std::string const& typestr) {
  size_t s = typestr.find("&") == 0 ? 1 : 0;
  size_t e = typestr.rfind("::");
  if (e != std::string::npos) {
    return typestr.substr(s, e - s);
  }
  return typestr.substr(s, typestr.length() - s);
}

/*static*/ std::string
TemplateExtract::getBarename(std::string const& typestr) {
  size_t s = typestr.rfind("::");
  if (s != std::string::npos) {
    s += 2;
    return typestr.substr(s, typestr.length() - s);
  }
  return typestr;
}

//
// DemanglerUtils
//

/*static*/ std::vector<std::string>
DemanglerUtils::splitString(std::string const& str, char delim) {
  std::stringstream ss;
  ss.str(str);

  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

/*static*/ std::string
DemanglerUtils::removeSpaces(std::string const& str) {
  std::string clean{str};

  clean.erase(
    std::remove(clean.begin(), clean.end(), ' '),
    clean.end());

  return clean;
}

}}} // end namespace vt::util::demangle
