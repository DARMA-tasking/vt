
#if !defined INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H
#define INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H

/*
 *  A soft error is treated like an ignored warning in certain build/runtime
 *  modes (e.g, some production cases) and an error in other modes (e.g., when
 *  debugging is enabled). In production modes, if the user configures it as so,
 *  the cost of these checks can be fully optimized out.
 */

#include "vt/configs/debug/debug_config.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/common.h"
#include "vt/configs/error/pretty_print_message.h"

#include <string>

#include <fmt/format.h>

namespace vt {

template <typename... Args>
inline void warning(
  std::string const& str, ErrorCodeType error, bool quit,
  std::string const& file, int const line, std::string const& func,
  Args&&... args
) {
  auto msg = "vtWarn() Invoked";
  std::string const buf = ::fmt::format(str, std::forward<Args>(args)...);
  auto inf = debug::stringizeMessage(msg,buf,"",file,line,func,error);
  if (quit) {
    return ::vt::output(inf,error,true,true,true);
  } else {
    return ::vt::output(inf,error,false,true,true);
  }
}

} /* end namespace vt */

#if backend_check_enabled(production)
  #define vtWarn(str,args...)
  #define vtWarnCode(error,str,args...)
  #define vtWarnIf(cond,str,args...)
  #define vtWarnIfCode(error,cond,str,args...)
  #define vtWarnFail(str,args...)
  #define vtWarnFailCode(error,str,args...)
#else
  #define vtWarn(str,args...)                                             \
    ::vt::warning(str,1,    false, DEBUG_LOCATION  outputArgsImpl(args));
  #define vtWarnCode(code,str,args...)                                    \
    ::vt::warning(str,code, false, DEBUG_LOCATION  outputArgsImpl(args));
  #define vtWarnFail(str,args...)                                         \
    ::vt::warning(str,1,    true,  DEBUG_LOCATION  outputArgsImpl(args));
  #define vtWarnFailCode(code,str,args...)                                \
    ::vt::warning(str,code, true,  DEBUG_LOCATION  outputArgsImpl(args));
  #define vtWarnIf(cond,str,args...)                                      \
    do {                                                                  \
      if (cond) {                                                         \
        vtWarn(str,args);                                                 \
      }                                                                   \
    } while (false)
  #define vtWarnIfCode(code,cond,str,args...)                             \
    do {                                                                  \
      if (cond) {                                                         \
        vtWarnCode(code,str,args);                                        \
      }                                                                   \
    } while (false)
  #define vtWarnIfNot(cond,str,args...)                                   \
    vtWarnIf(INVERT_COND(cond),str,args)
  #define vtWarnIfNotCode(code,cond,str,args...)                          \
    vtWarnIfCode(code,INVERT_COND(cond),str,args)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H*/
