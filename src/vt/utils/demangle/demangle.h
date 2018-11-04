
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
