
#if !defined INCLUDED_CONFIGS_ERROR_CODE_H
#define INCLUDED_CONFIGS_ERROR_CODE_H

#include "configs/error/code_class.h"

#include <cstdlib>

namespace vt { namespace error {

enum struct ErrorCode : int8_t {
  VrtColElmLocalExist = 0,
  VrtColElmProxyExist = 1,
  VrtColElmBufferSend = 2,
  VrtColElmBufferBcast = 3
};

}} /* end namespace vt::error */

#endif /*INCLUDED_CONFIGS_ERROR_CODE_H*/
