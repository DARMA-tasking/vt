
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
  event_t
  send_msg(
    node_t const& dest, handler_t const& han, MessageT* const msg,
    action_t next_action = nullptr
  ) {
    Envelope env(dest, han, envelope_type_t::Normal);
    msg->env.dest = dest;
    msg->env.han = han;
    auto const& msg_size = sizeof(MessageT);
    node_t const& this_node = the_context->get_node();
    auto const event_id = the_event->create_mpi_event_id(this_node);
    auto& holder = the_event->get_event_holder(event_id);
    MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());

    DEBUG_PRINT(
      "ActiveMessenger: sending: handler=%d, dest=%d, size=%d\n",
      han, dest, msg_size
    );

    MPI_Isend(
      msg, msg_size, MPI_BYTE, dest, 0, MPI_COMM_WORLD, mpi_event.get_request()
    );
    if (next_action != nullptr) {
      holder.attach_action(next_action);
    }
    return event_id;
  }

  void
  perform_triggered_actions() {
    the_event->test_events_trigger(mpi_event_tag);
    the_event->test_events_trigger(normal_event_tag);
  }

  bool
  try_process_incoming_message() {
    byte_t num_probe_bytes;
    MPI_Status stat;
    int flag;
    MPI_Iprobe(
      MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat
    );
    if (flag == 1) {
      MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);
      char* buf = new char[num_probe_bytes];
      MPI_Recv(
        buf, num_probe_bytes, MPI_BYTE, stat.MPI_SOURCE, stat.MPI_TAG,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
      );
      Message* msg = reinterpret_cast<Message*>(buf);
      auto handler = msg->env.han;
      //std::cout << "scheduler: handler=" << handler << std::endl;
      auto active_fun = the_registry->get_handler(handler);
      active_fun(msg);
      return true;
    } else {
      return false;
    }
  }

  void
  scheduler(int const& num_times = scheduler_default_num_times) {
    for (int i = 0; i < num_times; i++) {
      try_process_incoming_message();
      perform_triggered_actions();
    }
  }
};

extern std::unique_ptr<ActiveMessenger> the_msg;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_ACTIVE__*/
