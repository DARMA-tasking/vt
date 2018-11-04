
#if !defined INCLUDED_RUNTIME_RUNTIME_COMMON_H
#define INCLUDED_RUNTIME_RUNTIME_COMMON_H

#include "vt/config.h"

namespace vt { namespace runtime {

enum eRuntimeInstance {
  DefaultInstance = 0,
  OtherInstance = 1
};

using RuntimeInstType = eRuntimeInstance;

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_RUNTIME_COMMON_H*/
