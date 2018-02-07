
#include "config.h"
#include "pool/memory_pool_equal.h"
#include "pool/pool_header.h"

#include <vector>
#include <cstdint>
#include <cassert>

namespace vt { namespace pool {

template <int64_t num_bytes_t>
MemoryPoolEqual<num_bytes_t>::MemoryPoolEqual(SlotType const in_pool_size)
  : pool_size_(in_pool_size)
{
  resizePool();
}

template <int64_t num_bytes_t>
/*virtual*/ MemoryPoolEqual<num_bytes_t>::~MemoryPoolEqual() {
  debug_print(
    pool, node,
    "cur_slot_=%lld\n", cur_slot_
  );

  // for (auto i = 0; i < cur_slot_; i++) {
  //   debug_print_force(
  //     pool, node,
  //     "alloc never freed: ptr=%p, cur_slot_=%d\n", holder_.at(i), i
  //   );
  // }

  for (int i = cur_slot_; i < holder_.size(); i++) {
    free(holder_.at(i));
  }
}

template <int64_t num_bytes_t>
void* MemoryPoolEqual<num_bytes_t>::alloc(size_t const& sz) {
  if (cur_slot_ + 1 >= holder_.size()) {
    resizePool();
  }

  assert(
    cur_slot_+1 < holder_.size() and
    "Must be within pool size, add capability to grow"
  );

  auto const& slot = cur_slot_;
  void* const ptr = holder_[slot];
  void* const ptr_ret = HeaderManagerType::setHeader(sz, static_cast<char*>(ptr));

  debug_print(
    pool, node,
    "alloc ptr=%p, ptr_ret=%p cur_slot=%lld\n", ptr, ptr_ret, cur_slot_
  );

  cur_slot_++;

  return ptr_ret;
}

template <int64_t num_bytes_t>
void MemoryPoolEqual<num_bytes_t>::dealloc(void* const t) {
  debug_print(
    pool, node,
    "dealloc t=%p, cur_slot=%lld\n", t, cur_slot_
  );

  assert(
    cur_slot_ - 1 >= 0 and "Must be greater than zero"
  );

  auto t_char = static_cast<char*>(t);
  void* const ptr_actual = HeaderManagerType::getHeaderPtr(t_char);

  holder_[--cur_slot_] = ptr_actual;
}

template <int64_t num_bytes_t>
void MemoryPoolEqual<num_bytes_t>::resizePool() {
  SlotType const cur_size = holder_.size();
  SlotType const new_size = cur_size == 0 ? pool_size_ : cur_size * 2;

  holder_.resize(new_size);

  for (auto i = cur_size; i < holder_.size(); i++) {
    holder_[i] = static_cast<void*>(malloc(num_full_bytes_));
  }
}

template <int64_t num_bytes_t>
typename MemoryPoolEqual<num_bytes_t>::SlotType
MemoryPoolEqual<num_bytes_t>::getNumBytes() {
  return num_bytes_;
}

template struct MemoryPoolEqual<memory_size_small>;
template struct MemoryPoolEqual<memory_size_medium>;

}} //end namespace vt::pool
