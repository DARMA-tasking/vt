
#if ! defined __RUNTIME_TRANSPORT_CUSTOM_INDEX__
#define __RUNTIME_TRANSPORT_CUSTOM_INDEX__

#include "config.h"

#include <cstdint>

namespace vt { namespace index {

struct BaseIndex {

  // An index Must have equality defined
  bool operator==(CustomIndex const& other) const;

};

}} // end namespace vt::index

#endif /*__RUNTIME_TRANSPORT_CUSTOM_INDEX__*/
