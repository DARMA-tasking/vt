
#if ! defined __RUNTIME_TRANSPORT_CUSTOM_INDEX__
#define __RUNTIME_TRANSPORT_CUSTOM_INDEX__

#include "config.h"

#include <cstdint>

namespace vt { namespace index {

struct BaseIndex {
  using IndexSizeType = size_t;

  // An index must have a copy constructor
  BaseIndex(BaseIndex const&);

  // An index must have equality defined
  bool operator==(BaseIndex const& other) const;

  // An index must inform the runtime of its packed size
  IndexSizeType packedSize() const;

  // An index must inform the runtime if it is byte copyable
  bool isByteCopyable() const;
};

}} // end namespace vt::index

#endif /*__RUNTIME_TRANSPORT_CUSTOM_INDEX__*/
