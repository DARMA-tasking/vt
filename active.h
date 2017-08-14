
#if ! defined __RUNTIME_TRANSPORT_ACTIVE__
#define __RUNTIME_TRANSPORT_ACTIVE__

#include <cstdint>
#include <memory>
#include <iostream>
#include <mpi.h>

#include "common.h"
#include "event.h"
#include "registry.h"

namespace runtime {

struct ActiveMessenger {
  using byte_t = int32_t;

  ActiveMessenger() = default;

  template <typename MessageT>
  void
  set_term_message(MessageT* const msg) {
    msg->env.is_term = 1;
  }

  template <typename MessageT>
  event_t
  send_msg(
    node_t const& dest, handler_t const& han, MessageT* const msg,
    action_t next_action = nullptr
  ) {
    // setup envelope
    msg->env.dest = dest;
    msg->env.han = han;
    msg->env.type = envelope_type_t::Normal;
    msg->env.epoch = no_epoch;

    return send_msg_direct(dest, han, msg, sizeof(MessageT));
  }

  template <typename MessageT>
  event_t
  broadcast_msg(
    handler_t const& han, MessageT* const msg, action_t next_action = nullptr
  ) {
    return send_msg(-1, han, msg, next_action);
  }

  event_t
  send_msg_direct(
    node_t const& dest, handler_t const& han, Message* const msg,
    int const& msg_size, action_t next_action = nullptr
  );

  void
  perform_triggered_actions();

  bool
  try_process_incoming_message();

  void
  scheduler(int const& num_times = scheduler_default_num_times);

};

extern std::unique_ptr<ActiveMessenger> the_msg;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_ACTIVE__*/
