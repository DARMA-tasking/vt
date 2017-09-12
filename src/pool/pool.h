
#if ! defined __RUNTIME_TRANSPORT_POOL__
#define __RUNTIME_TRANSPORT_POOL__

#include "common.h"
#include "envelope.h"
#include "context.h"

#include <vector>
#include <cstdint>
#include <cassert>

namespace runtime { namespace pool {

template <int64_t num_bytes_t>
struct MemoryPoolEqual {
  using ContainerType = std::vector<void*>;
  using SlotType = int64_t;

  static constexpr SlotType const fst_pool_slot = 0;
  static constexpr SlotType const default_pool_size = 1024;

  // MemoryPoolEqual(slot_t const& in_num_bytes, slot_t const& in_pool_size = default_pool_size)
  //   : num_bytes(in_num_bytes), pool_size(in_pool_size)
  // {
  //   resize_pool();
  // }

  MemoryPoolEqual() {
    resize_pool();
  }

  void* alloc(size_t const& sz) {
    if (cur_slot_+1 >= holder_.size()) {
      resize_pool();
    }

    assert(
      cur_slot_+1 < holder_.size() and "Must be within pool size, add capability to grow"
    );

    auto const& slot = cur_slot_;
    void* const ptr = holder_[slot];

    *static_cast<size_t*>(ptr) = sz;

    void* const ptr_ret = static_cast<size_t*>(ptr) + 1;

    cur_slot_++;

    return ptr_ret;
  }

  void dealloc(void* const t) {
    assert(
      cur_slot_-1 >= 0 and "Must be greater than zero"
    );

    debug_print(
      pool, node,
      "%d: dealloc t=%p, cur_slot=%lld\n",
      the_context->get_node(), t, cur_slot
    );

    void* const ptr_actual = static_cast<size_t*>(t) - 1;

    holder_[--cur_slot_] = ptr_actual;
  }

  void resize_pool() {
    SlotType const cur_size = holder_.size();
    SlotType const new_size = cur_size == 0 ? pool_size_ : cur_size * 2;

    holder_.resize(new_size);

    for (auto i = cur_size; i < holder_.size(); i++) {
      holder_[i] = static_cast<void*>(malloc(num_full_bytes_));
    }
  }

  SlotType get_num_bytes() const {
    return num_bytes_;
  }

private:
  SlotType const num_bytes_ = num_bytes_t;
  SlotType const num_full_bytes_ = num_bytes_t + sizeof(size_t);

  SlotType pool_size_ = default_pool_size;
  SlotType cur_slot_ = fst_pool_slot;

  ContainerType holder_;
};

struct Pool {
  static constexpr size_t const small_msg_size_buf = sizeof(int64_t) * 4;

  MemoryPoolEqual<sizeof(EpochTagEnvelope) + small_msg_size_buf> small_msg;

  void*
  alloc(size_t const& num_bytes);

  void
  dealloc(void* const buf);

  bool
  size_is_large(size_t const& num_bytes);
};

}} //end namespace runtime::pool

namespace runtime {

extern std::unique_ptr<pool::Pool> the_pool;

template <typename MessageT, typename... Args>
MessageT* make_shared_message(Args&&... args) {
  MessageT* msg = new MessageT{args...};
  envelope_set_ref(msg->env, 1);
  return msg;
}

template <typename MessageT>
bool is_shared_message(MessageT* msg) {
  return envelope_get_ref(msg->env) != not_shared_message;
}

template <typename MessageT>
void message_convert_to_shared(MessageT* msg) {
  envelope_set_ref(msg->env, 1);
}

template <typename MessageT>
void message_set_unmanaged(MessageT* msg) {
  envelope_set_ref(msg->env, not_shared_message);
}

template <typename MessageT>
void message_ref(MessageT* msg) {
  envelope_ref(msg->env);
}

template <typename MessageT>
void message_deref(MessageT* msg) {
  envelope_deref(msg->env);

  debug_print(
    pool, node,
    "message_deref msg=%p, refs=%d\n", msg, envelope_get_ref(msg->env)
  );

  if (envelope_get_ref(msg->env) == 0) {
    delete msg;
  }
}

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_POOL__*/
