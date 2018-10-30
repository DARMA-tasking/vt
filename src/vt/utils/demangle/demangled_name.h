
#if !defined INCLUDED_UTILS_DEMANGLE_DEMANGLED_NAME_H
#define INCLUDED_UTILS_DEMANGLE_DEMANGLED_NAME_H

#include "config.h"

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
