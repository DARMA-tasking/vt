
#if !defined INCLUDED_DEMANGLE
#define INCLUDED_DEMANGLE

#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <cxxabi.h>
#include <assert.h>

#include "config.h"


namespace vt {
namespace demangle {

using StrContainerType = std::vector<std::string>;

struct DemanglerUtils {
  template<class T>
  static inline std::string getTypeName() {
    return typeid(T).name();
  }

  static inline std::string demangle(std::string const& name) {
    int status = -1;
    char *result = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
    return status == 0 ? std::string(result) : name;
  }

  template<typename T>
  static inline std::string getDemangledType() {
    auto const& type = getTypeName<T>();
    return demangle(type);
  }

  template<typename StringOut>
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
  using StrParsedOutType = std::tuple<std::string, std::string>;
  using UtilType = DemanglerUtils;

  static StrParsedOutType parseActiveFunctionName(std::string const& str);
};

struct ActiveFunctorDemangler {
  using StrParsedOutType = std::tuple<std::string, std::string>;
  using UtilType = DemanglerUtils;

  static StrParsedOutType parseActiveFunctorName(
      std::string const& name, std::string const& args
  );
};

}  // end namespace demangle
}  // end namespace vt

#endif  /*INCLUDED_DEMANGLE*/
