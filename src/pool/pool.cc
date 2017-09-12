
#include "pool.h"

namespace runtime { namespace pool {

bool Pool::size_is_large(size_t const& num_bytes) {
  return num_bytes > small_msg.get_num_bytes();
}

void* Pool::alloc(size_t const& num_bytes) {
  auto const& small_bytes = small_msg.get_num_bytes();

  void* ret = nullptr;

  bool const is_large = size_is_large(num_bytes);

  if (not is_large) {
    ret = small_msg.alloc(num_bytes);
  } else {
    ret = malloc(num_bytes + sizeof(size_t));
    *static_cast<size_t*>(ret) = num_bytes;
    ret = static_cast<size_t*>(ret) + 1;
  }

  debug_print(
    pool, node,
    "Pool::alloc of size=%zu, is_large=%s, small_bytes=%lld, ret=%p\n",
    num_bytes, is_large ? "true" : "false", small_bytes, ret
  );

  return ret;
}

void Pool::dealloc(void* const buf) {
  auto const& small_bytes = small_msg.get_num_bytes();

  void* const ptr_actual = static_cast<size_t*>(buf) - 1;
  auto const& actual_alloc_size = *static_cast<size_t*>(ptr_actual);

  bool const is_large = size_is_large(actual_alloc_size);

  debug_print(
    pool, node,
    "Pool::dealloc of buf=%p, is_large=%s, actual_alloc_size=%ld\n",
    buf, is_large ? "true" : "false", actual_alloc_size
  );

  if (not is_large) {
    small_msg.dealloc(buf);
  } else {
    free(ptr_actual);
  }
};

}} //end namespace runtime::pool

