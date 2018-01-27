
#if !defined INCLUDED_POOL_POOL_H
#define INCLUDED_POOL_POOL_H

#include "config.h"
#include "memory_pool_equal.h"

#include <vector>
#include <cstdint>
#include <cassert>

namespace vt { namespace pool {

struct Pool {
  using SizeType = size_t;

  enum struct ePoolSize {
    Small = 1,
    Medium = 2,
    Large = 3,
    Malloc = 4
  };

  MemoryPoolEqual<small_memory_pool_env_size> small_msg;
  MemoryPoolEqual<medium_memory_pool_env_size> medium_msg;

  void* alloc(size_t const& num_bytes);
  void dealloc(void* const buf);
  ePoolSize getPoolType(size_t const& num_bytes);
  SizeType remainingSize(void* const buf);
};

}} //end namespace vt::pool

namespace vt {

extern pool::Pool* thePool();

} //end namespace vt

#endif /*INCLUDED_POOL_POOL_H*/
