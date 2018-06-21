
#if !defined INCLUDED_DEMANGLE
#define INCLUDED_DEMANGLE

#include "config.h"

#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <cxxabi.h>
#include <assert.h>

namespace vt { namespace demangle {

using StrContainerType = std::vector<std::string>;

struct static_string {
  const char* const p_;
  const std::size_t sz_;

public:
  typedef const char* const_iterator;

  template <std::size_t N>
  constexpr static_string(const char(&a)[N]) noexcept
    : p_(a)
    , sz_(N-1)
  {}

  constexpr static_string(const char* p, std::size_t N) noexcept
    : p_(p)
    , sz_(N)
  {}

  constexpr const char* data() const noexcept {return p_;}
  constexpr std::size_t size() const noexcept {return sz_;}

  constexpr const_iterator begin() const noexcept {return p_;}
  constexpr const_iterator end()   const noexcept {return p_ + sz_;}

  constexpr char operator[](std::size_t n) const
  {
    return n < sz_ ? p_[n] : throw std::out_of_range("static_string");
  }
};

// inline std::ostream& operator<<(std::ostream& os, static_string const& s)
// {
//   return os.write(s.data(), s.size());
// }

template <class T>
constexpr static_string type_name() {
#ifdef __clang__
  static_string p = __PRETTY_FUNCTION__;
  return static_string(p.data() + 31, p.size() - 31 - 1);
#elif defined(__GNUC__)
  static_string p = __PRETTY_FUNCTION__;
#  if __cplusplus < 201402
  return static_string(p.data() + 36, p.size() - 36 - 1);
#  else
  return static_string(p.data() + 46, p.size() - 46 - 1);
#  endif
#elif defined(_MSC_VER)
  static_string p = __FUNCSIG__;
  return static_string(p.data() + 38, p.size() - 38 - 7);
#endif
}

struct DemanglerUtils {
  template <typename T>
  static inline std::string getTypeName() {
    std::string s{type_name<T>().data()};
    ::fmt::print("name={}\n",s);
    return s;
  }

  static inline std::string demangle(std::string const& name) {
    int status = -1;
    char *result = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
    return status == 0 ? std::string(result) : name;
  }

  template <typename T>
  static inline std::string getDemangledType() {
    auto const& type = getTypeName<T>();
    return demangle(type);
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

}}  // end namespace vt::demangle

#endif  /*INCLUDED_DEMANGLE*/
