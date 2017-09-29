
#if ! defined __RUNTIME_TRANSPORT_SERIALIZATION__
#define __RUNTIME_TRANSPORT_SERIALIZATION__

#include "config.h"

#include <cstdint>

namespace vt { namespace serialization {

enum struct eSerializationMode : int8_t {
  None = 0,
  Unpacking = 1,
  Packing = 2,
  Sizing = 3,
  Invalid = -1
};


}} //end namespace vt::serialization

#endif /*__RUNTIME_TRANSPORT_SERIALIZATION__*/
