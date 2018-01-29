
#include "config.h"
#include "pool/pool.h"
#include "pool/memory_pool_equal.h"

#include <cstdlib>
#include <cstdint>

namespace vt { namespace pool {

Pool::Pool()
  : small_msg(
      std::make_unique<MemoryPoolType<small_memory_pool_env_size>>()
    ),
    medium_msg(
      std::make_unique<MemoryPoolType<medium_memory_pool_env_size>>(64)
    )
{ }

Pool::ePoolSize Pool::getPoolType(size_t const& num_bytes) {
  if (num_bytes <= small_msg->getNumBytes()) {
    return ePoolSize::Small;
  } else if (num_bytes <= medium_msg->getNumBytes()) {
    return ePoolSize::Medium;
  } else {
    return ePoolSize::Malloc;
  }
}

void* Pool::alloc(size_t const& num_bytes) {
  void* ret = nullptr;

  ePoolSize const pool_type = getPoolType(num_bytes);

  if (pool_type == ePoolSize::Small) {
    ret = small_msg->alloc(num_bytes);
  } else if (pool_type == ePoolSize::Medium) {
    ret = medium_msg->alloc(num_bytes);
  } else {
    ret = std::malloc(num_bytes + sizeof(size_t));
    *static_cast<size_t*>(ret) = num_bytes;
    ret = static_cast<size_t*>(ret) + 1;
  }

  debug_print(
    pool, node,
    "Pool::alloc of size=%zu, type=%s, ret=%p\n",
    num_bytes, print_pool_type(pool_type), ret
  );

  return ret;
}

void Pool::dealloc(void* const buf) {
  void* const ptr_actual = static_cast<size_t*>(buf) - 1;
  auto const& actual_alloc_size = *static_cast<size_t*>(ptr_actual);

  ePoolSize const pool_type = getPoolType(actual_alloc_size);

  debug_print(
    pool, node,
    "Pool::dealloc of buf=%p, type=%s, actual_alloc_size=%ld\n",
    buf, print_pool_type(pool_type), actual_alloc_size
  );

  if (pool_type == ePoolSize::Small) {
    small_msg->dealloc(buf);
  } else if (pool_type == ePoolSize::Medium) {
    medium_msg->dealloc(buf);
  } else {
    std::free(ptr_actual);
  }
};

Pool::SizeType Pool::remainingSize(void* const buf) {
  void* const ptr_actual = static_cast<size_t*>(buf) - 1;
  auto const& actual_alloc_size = *static_cast<size_t*>(ptr_actual);

  ePoolSize const pool_type = getPoolType(actual_alloc_size);

  if (pool_type == ePoolSize::Small) {
    return small_msg->getNumBytes() - actual_alloc_size;
  } else if (pool_type == ePoolSize::Medium) {
    return medium_msg->getNumBytes() - actual_alloc_size;
  } else {
    return 0;
  }
}

}} //end namespace vt::pool
