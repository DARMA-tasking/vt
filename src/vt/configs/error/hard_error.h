
#if !defined INCLUDED_CONFIGS_ERROR_HARD_ERROR_H
#define INCLUDED_CONFIGS_ERROR_HARD_ERROR_H

/*
 *  A hard error is always checked and leads to failure in any mode if
 *  triggered
 */

#include "configs/debug/debug_config.h"
#include "configs/types/types_type.h"

#include <string>

namespace vt {

// Forward declare abort and output signatures, defined in collective ops
void abort(std::string const str, ErrorCodeType const code);

inline void error(std::string const& str, ErrorCodeType error) {
  return ::vt::abort(str,error);
}

} /* end namespace vt */

#if backend_check_enabled(production)
  #define vtAbort(str)           ::vt::error(str,1);
  #define vtAbortCode(str,error) ::vt::error(str,error);
  #define vtAbortIf(cond,str)
#else
  #define vtAbort(str)           ::vt::error(str,1);
  #define vtAbortCode(str,error) ::vt::error(str,error);
  #define vtAbortIf(cond,str)                     \
    do {                                          \
      if ((cond)) {                               \
        vtAbort(str);                             \
      }                                           \
    } while (false)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_HARD_ERROR_H*/
