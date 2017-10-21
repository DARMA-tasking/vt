
#if !defined INCLUDED_POOL_POOL_H
#define INCLUDED_POOL_POOL_H

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

extern pool::Pool* thePool();

} //end namespace vt

#endif /*INCLUDED_POOL_POOL_H*/
