
#if ! defined __RUNTIME_TRANSPORT_EXAMPLE_INDEX__
#define __RUNTIME_TRANSPORT_EXAMPLE_INDEX__

#include "config.h"

#include <cstdint>

namespace vt { namespace index {

/*
 * This index (ExampleIndex) exists for pedagogical purposes only: to demonstrate
 * an example vt::index that conforms to the correct interface. If the detector
 * is enabled, this can be checked at compile time.
 */

struct ExampleIndex {
  using IndexSizeType = size_t;

  // An index must have a default constructor
  ExampleIndex() = default;

  // An index must have a copy constructor
  ExampleIndex(ExampleIndex const&) = default;

  // An index must have equality defined
  bool operator==(ExampleIndex const& other) const;

  // An index must inform the runtime of its packed size
  IndexSizeType packedSize() const;

  // An index must inform the runtime if it is byte copyable
  bool isByteCopyable() const;
};

#if backend_check_enabled(detector)
  #include "index/index_traits.h"

  static_assert(
    vt::index::IndexTraits<ExampleIndex>::is_index,
    "vt::index::ExampleIndex must follow the Index concept"
  );
#endif

}} // end namespace vt::index

#endif /*__RUNTIME_TRANSPORT_EXAMPLE_INDEX__*/
