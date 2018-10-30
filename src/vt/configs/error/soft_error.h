
#if !defined INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H
#define INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H

/*
 *  A soft error is treated like an ignored warning in certain build/runtime
 *  modes (e.g, some production cases) and an error in other modes (e.g., when
 *  debugging is enabled). In production modes, if the user configures it as so,
 *  the cost of these checks can be fully optimized out.
 */

#include "configs/debug/debug_config.h"
#include "configs/types/types_type.h"

#include <string>

namespace vt {

// Forward declare abort and output signatures, defined in collective ops
void abort(std::string const str, ErrorCodeType const code);
void output(std::string const str, ErrorCodeType const code);

inline void warning(std::string const& str, ErrorCodeType error, bool quit) {
  if (quit) {
    return ::vt::abort(str,error);
  } else {
    return ::vt::output(str,error);
  }
}

} /* end namespace vt */

#if backend_check_enabled(production)
  #define vtWarn(str)
  #define vtWarnCode(str,error)
  #define vtWarnIf(cond,str)
  #define vtWarnIfCode(cond,str,error)
  #define vtWarnFail(str)
  #define vtWarnFailCode(str,error)
#else
  #define vtWarn(str)               ::vt::warning(str,1,false);
  #define vtWarnCode(str,error)     ::vt::warning(str,error,false);
  #define vtWarnFail(str)           ::vt::warning(str,1,true);
  #define vtWarnFailCode(str,error) ::vt::warning(str,error,true);
  #define vtWarnIf(cond,str)                            \
    do {                                                \
      if (cond) {                                       \
        vtWarn(str);                                    \
      }                                                 \
    } while (false)
  #define vtWarnIfCode(cond,str,error)                  \
    do {                                                \
      if (cond) {                                       \
        vtWarnCode(str,error);                          \
      }                                                 \
    } while (false)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_SOFT_ERROR_H*/
