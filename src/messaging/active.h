
#if ! defined __RUNTIME_TRANSPORT_ACTIVE__
#define __RUNTIME_TRANSPORT_ACTIVE__

#include <cstdint>
#include <memory>
#include <mpi.h>

#include "common.h"
#include "function.h"
#include "event.h"
#include "registry.h"
#include "auto_registry_interface.h"

#include <type_traits>
#include <tuple>
#include <vector>
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
  NodeType recv_node = uninitialized_destination;

  PendingRecv(
    void* in_user_buf, rdma_continuation_del_t in_cont,
    action_t in_dealloc_user_buf, NodeType node
  ) : user_buf(in_user_buf), cont(in_cont), dealloc_user_buf(in_dealloc_user_buf),
      recv_node(node)
  { }
};

struct BufferedActiveMsg {
  using message_t = ShortMessage*;

  message_t buffered_msg;
  NodeType from_node;

  BufferedActiveMsg(
    message_t const& in_buffered_msg, NodeType const& in_from_node
  ) : buffered_msg(in_buffered_msg), from_node(in_from_node)
  { }
};

struct ActiveMessenger {
  using buffered_msg_t = BufferedActiveMsg;
  using message_t = ShortMessage*;
  using byte_t = int32_t;
  using pending_recv_t = PendingRecv;
  using send_data_ret_t = std::tuple<event_t, tag_t>;
  using send_fn_t = std::function<send_data_ret_t(rdma_get_t,NodeType,tag_t,action_t)>;
  using user_send_fn_t = std::function<void(send_fn_t)>;
  using container_pending_t = std::unordered_map<tag_t, pending_recv_t>;
  using msg_cont_t = std::list<buffered_msg_t>;
  using container_waiting_handler_t = std::unordered_map<handler_t, msg_cont_t>;
  using ready_han_tag_t = std::tuple<handler_t, tag_t>;
  using maybe_ready_t = std::vector<ready_han_tag_t>;
  using handler_manager_t = HandlerManager;

  ActiveMessenger() = default;

  template <typename MessageT>
  void set_term_message(MessageT* const msg) {
    set_term_type(msg->env);
  }

  template <typename MessageT>
  void set_epoch_message(MessageT* const msg, epoch_t const& epoch) {
    envelope_set_epoch(msg->env, epoch);
  }

  template <typename MessageT>
  void set_tag_message(MessageT* const msg, tag_t const& tag) {
    envelope_set_tag(msg->env, tag);
  }

  /*----------------------------------------------------------------------------
   *            Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   *
   * Send message  to pre-registered active message handler.
   *
   *   void my_handler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *   handler_t const han = register_new_handler(my_handler);
   *
   *   MyMsg* msg = make_shared_message<MyMsg>(156);
   *   the_msg->send_msg(29, han, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT>
  event_t send_msg(
    NodeType const& dest, handler_t const& han, MessageT* const msg,
    action_t next_action = nullptr
  ) {
    envelope_setup(msg->env, dest, han);
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  template <typename MessageT>
  event_t send_msg(
    handler_t const& han, MessageT* const msg, tag_t const& tag = no_tag,
    action_t next_action = nullptr
  ) {
    auto const& dest = handler_manager_t::get_handler_node(han);
    assert(
      dest != uninitialized_destination and
      "Destination must be known in handler"
    );
    envelope_setup(msg->env, dest, han);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  /*
   *----------------------------------------------------------------------------
   *          End Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *              Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   *
   * Send message using a type-safe function handler. This is the predominant
   * way that the messenger is expected to be used.
   *
   *   void my_handler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *  the_msg->send_msg<MyMsg, my_handler>(1, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, action_any_function_t<MessageT>* f>
  event_t broadcast_msg(
    MessageT* const msg, tag_t const& tag = no_tag, action_t next_action = nullptr
  ) {
    handler_t const& han = auto_registry::make_auto_handler<MessageT,f>(msg);
    auto const& this_node = the_context->get_node();
    set_broadcast_type(msg->env);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg(this_node, han, msg, next_action);
  }

  template <typename MessageT, action_any_function_t<MessageT>* f>
  event_t broadcast_msg(MessageT* const msg, action_t act) {
    return broadcast_msg<MessageT,f>(msg,no_tag,act);
  }

  template <typename MessageT, action_any_function_t<MessageT>* f>
  event_t send_msg(
    NodeType const& dest, MessageT* const msg, tag_t const& tag = no_tag,
    action_t next_action = nullptr
  ) {
    handler_t const& han = auto_registry::make_auto_handler<MessageT,f>(msg);
    envelope_setup(msg->env, dest, han);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  template <typename MessageT, action_any_function_t<MessageT>* f>
  event_t send_msg(NodeType const& dest, MessageT* const msg, action_t act) {
    return send_msg<MessageT,f>(dest,msg,no_tag,act);
  }

  /*
   *----------------------------------------------------------------------------
   *             End Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *                 Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   *
   * Send message using basic function handler. These handlers are NOT type-safe
   * and require the user to cast their message to the correct type as so:
   *
   *   void basic_handler(runtime::BaseMessage* msg_in) {
   *     MyMsg* msg = static_cast<MyMsg*>(msg_in);
   *     ...
   *   }
   *
   *  the_msg->send_msg<basic_handler, MyMsg>(1, msg);
   *
   * Most likely this will be deprecated unless there is a use for this, since
   * type safety does not cost anything in terms of overhead (either at runtime
   * or compile-time).
   *
   *----------------------------------------------------------------------------
   */

  template <active_basic_function_t* f, typename MessageT>
  event_t broadcast_msg(
    MessageT* const msg, tag_t const& tag = no_tag, action_t next_action = nullptr
  ) {
    handler_t const& han = auto_registry::make_auto_handler<MessageT,f>(msg);
    auto const& this_node = the_context->get_node();
    set_broadcast_type(msg->env);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg(this_node, han, msg, next_action);
  }

  template <active_basic_function_t* f, typename MessageT>
  event_t broadcast_msg(MessageT* const msg, action_t act) {
    return broadcast_msg<f,MessageT>(msg,no_tag,act);
  }

  template <active_basic_function_t* f, typename MessageT>
  event_t send_msg(
    NodeType const& dest, MessageT* const msg, tag_t const& tag = no_tag,
    action_t next_action = nullptr
  ) {
    handler_t const& han = auto_registry::make_auto_handler<MessageT,f>(msg);
    envelope_setup(msg->env, dest, han);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  template <active_basic_function_t* f, typename MessageT>
  event_t send_msg(NodeType const& dest, MessageT* const msg, action_t act) {
    return send_msg<f,MessageT>(dest,msg,no_tag,act);
  }

  /*
   *----------------------------------------------------------------------------
   *              End Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *                       Send Message Functor Variants
   *----------------------------------------------------------------------------
   *
   * Send message Functor variants that cause an active message to trigger a
   * user-defined functor such as:
   *
   *   struct X {
   *     void operator()(MyMsg* msg) const { ... };
   *   };
   *
   *----------------------------------------------------------------------------
   */

  template <typename FunctorT, typename MessageT>
  event_t broadcast_msg(
    MessageT* const msg, tag_t const& tag = no_tag, action_t next_action = nullptr
  ) {
    handler_t const& han =
      auto_registry::make_auto_handler_functor<FunctorT, true, MessageT*>();
    set_broadcast_type(msg->env);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg(the_context->get_node(), han, msg, next_action);
  }

  template <typename FunctorT, typename MessageT>
  event_t broadcast_msg(MessageT* const msg, action_t act) {
    return broadcast_msg<FunctorT,MessageT>(msg,no_tag,act);
  }

  template <typename FunctorT, typename MessageT>
  event_t send_msg(
    NodeType const& dest, MessageT* const msg, tag_t const& tag = no_tag,
    action_t next_action = nullptr
  ) {
    handler_t const& han =
      auto_registry::make_auto_handler_functor<FunctorT, true, MessageT*>();
    envelope_setup(msg->env, dest, han);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    return send_msg_direct(han, msg, sizeof(MessageT), next_action);
  }

  template <typename FunctorT, typename MessageT>
  event_t send_msg(NodeType const& dest, MessageT* const msg, action_t act) {
    return send_msg<FunctorT,MessageT>(dest,msg,no_tag,act);
  }

  /*
   *----------------------------------------------------------------------------
   *                      End Send Message Functor Variants
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *                     Send Data Message (includes payload)
   *----------------------------------------------------------------------------
   *
   * Send message that includes a payload that can be arbitrary data that is
   * coordinated by the system
   *
   *----------------------------------------------------------------------------
   */
  template <typename MessageT>
  event_t send_msg(
    NodeType const& dest, handler_t const& han, MessageT* const msg,
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

  template <typename MessageT, action_any_function_t<MessageT>* f>
  event_t send_msg(
    NodeType const& dest, MessageT* const msg, user_send_fn_t send_payload_fn,
    action_t next_action = nullptr
  ) {
    handler_t const& han = auto_registry::make_auto_handler<MessageT,f>(msg);
    return send_msg<MessageT>(dest, han, msg, send_payload_fn, next_action);
  }

  send_data_ret_t send_data(
    rdma_get_t const& ptr, NodeType const& dest, tag_t const& tag,
    action_t next_action = nullptr
  );

  bool recv_data_msg(
    tag_t const& tag, NodeType const& node, rdma_continuation_del_t next = nullptr
  );

  bool recv_data_msg(
    tag_t const& tag, NodeType const& recv_node, bool const& enqueue,
    rdma_continuation_del_t next = nullptr
  );

  bool recv_data_msg_buffer(
    void* const user_buf, tag_t const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    action_t dealloc_user_buf = nullptr, rdma_continuation_del_t next = nullptr
  );

  template <typename MessageT>
  event_t broadcast_msg(
    handler_t const& han, MessageT* const msg, action_t next_action = nullptr
  ) {
    auto const& this_node = the_context->get_node();
    set_broadcast_type(msg->env);
    return send_msg(this_node, han, msg, next_action);
  }

  event_t send_msg_direct(
    handler_t const& han, BaseMessage* const msg, int const& msg_size,
    action_t next_action = nullptr
  );

  /*
   *----------------------------------------------------------------------------
   *                           End Send Data Message
   *----------------------------------------------------------------------------
   */

   /*----------------------------------------------------------------------------
   *                            Send Message Callback
   *----------------------------------------------------------------------------
   *
   * Send message *callback* variants (automatically allow user to callback upon
   * message arrival)
   *
   *----------------------------------------------------------------------------
   */
  template <typename MessageT, action_any_function_t<MessageT>* f>
  event_t send_msg_callback(
    NodeType const& dest, MessageT* const msg, active_function_t fn
  ) {
    handler_t const& this_han = register_new_handler(fn, no_tag);
    auto cb = static_cast<CallbackMessage*>(msg);
    cb->set_callback(this_han);
    return send_msg<MessageT,f>(dest, msg, no_tag, nullptr);
  }

  template <typename MessageT>
  void send_msg_callback(
    handler_t const& han, NodeType const& dest, MessageT* const msg,
    active_function_t fn
  ) {
    handler_t const& this_han = register_new_handler(fn, no_tag);
    auto cb = static_cast<CallbackMessage*>(msg);
    cb->set_callback(this_han);
    send_msg(dest, han, msg);
  }

  template <typename MessageT>
  void send_callback(MessageT* const msg) {
    auto const& han_callback = get_current_callback();
    send_msg(han_callback, msg);
  }

  /*
   *----------------------------------------------------------------------------
   *                        End Send Message Callback
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, action_any_function_t<MessageT>* f>
  void trigger(std::function<void(runtime::BaseMessage*)> fn) {
    handler_t const& han = auto_registry::make_auto_handler<MessageT,f>(nullptr);
    printf("trigger: han=%d\n", han);
    the_registry->save_trigger(han, /*reinterpret_cast<active_function_t>(*/fn);
  }

  void
  perform_triggered_actions();

  bool
  try_process_incoming_message();

  bool
  process_data_msg_recv();

  bool
  scheduler();

  bool
  is_local_term();

  handler_t
  register_new_handler(active_function_t fn, tag_t const& tag = no_tag);

  void
  swap_handler_fn(
    handler_t const& han, active_function_t fn, tag_t const& tag = no_tag
  );

  void
  unregister_handler_fn(handler_t const& han, tag_t const& tag = no_tag);

  void
  register_handler_fn(
    handler_t const& han, active_function_t fn, tag_t const& tag = no_tag
  );

  handler_t
  collective_register_handler(active_function_t fn, tag_t const& tag = no_tag);

  handler_t
  get_current_handler();

  handler_t
  get_current_callback();

  NodeType
  get_from_node_current_handler();

  bool
  deliver_active_msg(message_t msg, NodeType const& from_node, bool insert);

  void
  deliver_pending_msgs_on_han(handler_t const& han, tag_t const& tag = no_tag);

  void
  process_maybe_ready_han_tag();

private:
  handler_t current_handler_context = uninitialized_handler;
  handler_t current_callback_context = uninitialized_handler;
  NodeType current_node_context = uninitialized_destination;

  maybe_ready_t maybe_ready_tag_han;

  container_waiting_handler_t pending_handler_msgs;

  container_pending_t pending_recvs;

  tag_t cur_direct_buffer_tag = starting_direct_buffer_tag;
};

extern std::unique_ptr<ActiveMessenger> the_msg;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_ACTIVE__*/
