
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"
#include "pool.h"

namespace runtime {

struct BaseMessage { };

template <typename EnvelopeT>
struct ActiveMessage : BaseMessage {
  using envelope_t = EnvelopeT;
  envelope_t env;

  ActiveMessage() {
    envelope_init_empty(env);
  }

  static void*
  operator new(std::size_t sz) {
    return the_pool->alloc(sz);
  }

  static void
  operator delete(void* ptr) {
    return the_pool->dealloc(ptr);
  }
};

template <typename EnvelopeT>
struct GetActiveMessage : ActiveMessage<EnvelopeT> {

  GetActiveMessage()
    : ActiveMessage<EnvelopeT>()
  { }

  rdma_handle_t rdma_handle = no_rdma_handle;
  rdma_handler_t rdma_handler = uninitialized_rdma_handler;
  bool is_user_msg = false;
};

using ShortMessage = ActiveMessage<Envelope>;
using EpochMessage = ActiveMessage<EpochEnvelope>;
using EpochTagMessage = ActiveMessage<EpochTagEnvelope>;

using GetMessage = GetActiveMessage<EpochTagEnvelope>;
//using PutMessage = GetActiveMessage<EpochTagEnvelope>;

// default runtime::Message includes tag and epoch
using Message = EpochTagMessage;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
