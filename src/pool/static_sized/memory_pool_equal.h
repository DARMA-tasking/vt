
#if !defined INCLUDED_POOL_STATIC_SIZED_MEMORY_POOL_EQUAL_H
#define INCLUDED_POOL_STATIC_SIZED_MEMORY_POOL_EQUAL_H

#include "config.h"
#include "messaging/envelope.h"
#include "context/context.h"
#include "pool/header/pool_header.h"

#include <vector>
#include <cstdint>

namespace vt { namespace pool {

static constexpr size_t const small_msg_size_buf =
  sizeof(int64_t)*8 - sizeof(EpochTagEnvelope);
static constexpr size_t const memory_size_small =
  sizeof(EpochTagEnvelope) + small_msg_size_buf;

static constexpr size_t const medium_msg_size_buf =
  sizeof(int64_t)*128 - sizeof(EpochTagEnvelope);
static constexpr size_t const memory_size_medium =
  sizeof(EpochTagEnvelope) + medium_msg_size_buf;

template <int64_t num_bytes_t>
struct MemoryPoolEqual {
  using ContainerType = std::vector<void*>;
  using SlotType = int64_t;
  using HeaderType = Header;
  using HeaderManagerType = HeaderManager;

  static constexpr SlotType const fst_pool_slot = 0;
  static constexpr SlotType const default_pool_size = 1024;

  MemoryPoolEqual(SlotType const in_pool_size = default_pool_size);

  virtual ~MemoryPoolEqual();

  void* alloc(size_t const& sz);
  void dealloc(void* const t);
  void resizePool();
  SlotType getNumBytes();

private:
  SlotType const num_bytes_ = num_bytes_t;
  SlotType const num_full_bytes_ = num_bytes_t + sizeof(size_t);

  SlotType pool_size_ = default_pool_size;
  SlotType cur_slot_ = fst_pool_slot;

  ContainerType holder_;
};

}} //end namespace vt::pool

#endif /*INCLUDED_POOL_STATIC_SIZED_MEMORY_POOL_EQUAL_H*/
