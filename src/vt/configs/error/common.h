
#if !defined INCLUDED_CONFIGS_ERROR_COMMON_H
#define INCLUDED_CONFIGS_ERROR_COMMON_H

#include "meld_headers.h"
#include "configs/types/types_type.h"

#define outputArgsImpl(args...)                                         \
  meld_if_stmt(                                                         \
    meld_to_bool(_meld_is_empty(args))                                  \
  )()(,args)                                                            \

#define INVERT_COND(cond) (!(cond))
#define DEBUG_LOCATION __FILE__,__LINE__,__func__

#include <string>

namespace vt {

// Forward declare abort and output signatures, defined in collective ops
void abort(std::string const str, ErrorCodeType const code);
void output(std::string const str, ErrorCodeType const code, bool, bool);

} /* end namespace vt */

#endif /*INCLUDED_CONFIGS_ERROR_COMMON_H*/
