/*
//@HEADER
// ************************************************************************
//
//                          demangle.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_UTILS_DEMANGLE_DEMANGLE_H
#define INCLUDED_UTILS_DEMANGLE_DEMANGLE_H

#include "vt/config.h"
#include "vt/utils/string/static.h"
#include "vt/utils/demangle/get_type_name.h"
#include "vt/utils/demangle/demangled_name.h"

#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <iterator>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <assert.h>

namespace vt { namespace util { namespace demangle {

using StrContainerType = std::vector<std::string>;

struct DemanglerUtils {
  template <typename T>
  static inline std::string getTypeName() {
    std::string s{type_name<T>().data()};
    return s;
  }

  template <typename StringOut>
  static inline void splitString(
    std::string const& s, char delim, StringOut result
  ) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
      *(result++) = item;
    }
  }

  static inline StrContainerType splitString(
    std::string const& str, char delim
  ) {
    StrContainerType elems;
    splitString(str, delim, std::back_inserter(elems));
    return elems;
  }

  static inline std::string removeSpaces(std::string const& str) {
    StrContainerType const& str_split = splitString(str, ' ');
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
 *  vt::auto_registry::Runnable<
 *    vt::auto_registry::FunctorAdapter<
 *      void (vt::term::TermCounterMsg*),
 *      &(vt::term::TerminationDetector::propagate_epoch_handler(
 *        vt::term::TermCounterMsg*)
 *       )
 *    >
 *  >
 */
struct ActiveFunctionDemangler {
  using StrParsedOutType = DemangledName;
  using UtilType = DemanglerUtils;

  static StrParsedOutType parseActiveFunctionName(std::string const& str);
};

struct ActiveFunctorDemangler {
  using StrParsedOutType = DemangledName;
  using UtilType = DemanglerUtils;

  static StrParsedOutType parseActiveFunctorName(
      std::string const& name, std::string const& args
  );
};

}}} // end namespace vt::util::demangle

#endif /*INCLUDED_UTILS_DEMANGLE_DEMANGLE_H*/
