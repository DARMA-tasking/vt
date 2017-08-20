
#if ! defined __RUNTIME_TRANSPORT_ACTIVE__
#define __RUNTIME_TRANSPORT_ACTIVE__

#include <cstdint>
#include <memory>
#include <iostream>
#include <mpi.h>

#include "common.h"
#include "function.h"
#include "event.h"
#include "registry.h"

#include <type_traits>
#include <tuple>
#include <unordered_map>

namespace runtime {

using mpi_tag_t = int;

enum class MPITag : mpi_tag_t {
  ActiveMsgTag = 1,
  DataMsgTag = 2
};

static constexpr tag_t const starting_direct_buffer_tag = 1000;

struct PendingRecv {
  void* user_buf = nullptr;
  rdma_continuation_del_t cont = nullptr;
  action_t dealloc_user_buf = nullptr;

  PendingRecv(
    void* in_user_buf, rdma_continuation_del_t in_cont,
    action_t in_dealloc_user_buf
  ) : user_buf(in_user_buf), cont(in_cont), dealloc_user_buf(in_dealloc_user_buf)
  { }
};

struct ActiveMessenger {
  using byte_t = int32_t;
  using pending_recv_t = PendingRecv;
  using send_data_ret_t = std::tuple<event_t, tag_t>;
  using send_fn_t = std::function<send_data_ret_t(rdma_get_t,node_t,tag_t,action_t)>;
  using user_send_fn_t = std::function<void(send_fn_t)>;
  using container_pending_t = std::unordered_map<tag_t, pending_recv_t>;

  ActiveMessenger() = default;

  template <typename MessageT>
  void
  set_term_message(MessageT* const msg) {
    set_term_type(msg->env);
  }

  template <typename MessageT>
  void
  set_epoch_message(MessageT* const msg, epoch_t const& epoch) {
    envelope_set_epoch(msg->env, epoch);
  }

  template <typename MessageT>
  void
  set_tag_message(MessageT* const msg, tag_t const& tag) {
    envelope_set_tag(msg->env, tag);
  }

  template <typename MessageT>
  event_t
  send_msg(
    node_t const& dest, handler_t const& han, MessageT* const msg,
    action_t next_action = nullptr
  ) {
    // setup envelope
    envelope_setup(msg->env, dest, han);
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  template <typename MessageT>
  event_t
  send_msg(
    handler_t const& han, MessageT* const msg, action_t next_action = nullptr
  ) {
    auto const& dest = the_registry->get_handler_node(han);
    assert(
      dest != uninitialized_destination and
      "Destination must be known in handler"
    );
    // setup envelope
    envelope_setup(msg->env, dest, han);
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  send_data_ret_t
  send_data(
    rdma_get_t const& ptr, node_t const& dest, tag_t const& tag,
    action_t next_action = nullptr
  );

  bool
  recv_data_msg(
    tag_t const& tag, rdma_continuation_del_t next = nullptr
  );

  bool
  recv_data_msg(
    tag_t const& tag, bool const& enqueue, rdma_continuation_del_t next = nullptr
  );

  bool
  recv_data_msg_buffer(
    void* const user_buf, tag_t const& tag, bool const& enqueue,
    action_t dealloc_user_buf = nullptr, rdma_continuation_del_t next = nullptr
  );

  template <typename MessageT>
  event_t
  send_msg(
    node_t const& dest, handler_t const& han, MessageT* const msg,
    user_send_fn_t send_payload_fn, action_t next_action = nullptr
  ) {
    using namespace std::placeholders;

    // must send first so action payload function runs before the send
    auto f = std::bind(&ActiveMessenger::send_data, this, _1, _2, _3, _4);
    send_payload_fn(f);

    // setup envelope
    envelope_setup(msg->env, dest, han);
    auto const& ret = send_msg_direct(han, msg, sizeof(MessageT), next_action);

    return ret;
  }

  template <typename MessageT>
  event_t
  broadcast_msg(
    handler_t const& han, MessageT* const msg, action_t next_action = nullptr
  ) {
    auto const& this_node = the_context->get_node();
    set_broadcast_type(msg->env);
    return send_msg(this_node, han, msg, next_action);
  }

  event_t
  send_msg_direct(
    handler_t const& han, BaseMessage* const msg, int const& msg_size,
    action_t next_action = nullptr
  );

  void
  check_term_single_node();

  void
  perform_triggered_actions();

  bool
  try_process_incoming_message();

  void
  process_data_msg_recv();

  void
  scheduler(int const& num_times = scheduler_default_num_times);

  handler_t
  register_new_handler(active_function_t fn);

  handler_t
  collective_register_handler(active_function_t fn);

private:
  container_pending_t pending_recvs;

  tag_t cur_direct_buffer_tag = starting_direct_buffer_tag;
};

extern std::unique_ptr<ActiveMessenger> the_msg;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_ACTIVE__*/
