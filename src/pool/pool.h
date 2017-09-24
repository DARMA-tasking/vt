
#if ! defined __RUNTIME_TRANSPORT_POOL__
#define __RUNTIME_TRANSPORT_POOL__

#include "config.h"
#include "memory_pool_equal.h"

#include <vector>
#include <cstdint>
#include <cassert>

namespace vt { namespace pool {

struct Pool {
  MemoryPoolEqual<memory_pool_env_size> small_msg;

  void* alloc(size_t const& num_bytes);
  void dealloc(void* const buf);
  bool sizeIsLarge(size_t const& num_bytes);
};

}} //end namespace vt::pool

namespace vt {

extern std::unique_ptr<pool::Pool> thePool;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_POOL__*/
