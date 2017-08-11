
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
    msg->env.dest = dest;
    msg->env.han = han;
    msg->env.type = envelope_type_t::Normal;

    auto const& msg_size = sizeof(MessageT);

    // DEBUG_PRINT(
    //   "ActiveMessenger: sending: handler=%d, dest=%d, size=%d\n",
    //   han, dest, msg_size
    // );

    return send_msg_direct(dest, han, msg, msg_size);
  }

  event_t
  send_msg_direct(
    node_t const& dest, handler_t const& han, Message* const msg,
    int const& msg_size, action_t next_action = nullptr
  ) {
    node_t const& this_node = the_context->get_node();

    DEBUG_PRINT(
      "send_msg_direct: dest=%d, han=%d, msg=%p\n",
      dest, han, msg
    );

    if (dest != -1) {
      auto const event_id = the_event->create_mpi_event_id(this_node);
      auto& holder = the_event->get_event_holder(event_id);
      MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());

      MPI_Isend(
        msg, msg_size, MPI_BYTE, dest, 0, MPI_COMM_WORLD, mpi_event.get_request()
      );

      if (next_action != nullptr) {
        holder.attach_action(next_action);
      }

      return event_id;
    } else {
      if (msg->env.broadcast_root == -1) {
        msg->env.broadcast_root = this_node;
      }

      auto const& l1 = (this_node - msg->env.broadcast_root)*2+1;
      auto const& l2 = (this_node - msg->env.broadcast_root)*2+2;

      auto const& num_nodes = the_context->get_num_nodes();

      if (l1 >= num_nodes && l2 >= num_nodes) {
        if (next_action != nullptr) { next_action(); }
        return -1;
      }

      DEBUG_PRINT(
        "broadcast_msg: l1=%d, l2=%d, broadcast_root=%d, num_nodes=%d\n",
        l1, l2, msg->env.broadcast_root, num_nodes
      );

      auto const parent_event_id = the_event->create_parent_event_id(this_node);
      auto& parent_holder = the_event->get_event_holder(parent_event_id);
      ParentEvent& parent_event = *static_cast<ParentEvent*>(parent_holder.get_event());
      if (next_action != nullptr) {
        parent_holder.attach_action(next_action);
      }

      if (l1 < num_nodes) {
        auto const event_id1 = the_event->create_mpi_event_id(this_node);
        auto& holder1 = the_event->get_event_holder(event_id1);
        MPIEvent& mpi_event1 = *static_cast<MPIEvent*>(holder1.get_event());

        DEBUG_PRINT(
          "broadcast_msg: sending to l1=%d, l2=%d, broadcast_root=%d, event_id=%lld\n",
          l1, l2, msg->env.broadcast_root, event_id1
        );
        MPI_Isend(
          msg, msg_size, MPI_BYTE, l1, 0, MPI_COMM_WORLD, mpi_event1.get_request()
        );
        parent_event.add_event(event_id1);
      }
      if (l2 < num_nodes) {
        auto const event_id2 = the_event->create_mpi_event_id(this_node);
        auto& holder2 = the_event->get_event_holder(event_id2);
        MPIEvent& mpi_event2 = *static_cast<MPIEvent*>(holder2.get_event());
        DEBUG_PRINT(
          "broadcast_msg: sending to l2=%d, l1=%d, broadcast_root=%d, event_id=%lld\n",
          l2, l1, msg->env.broadcast_root, event_id2
        );
        MPI_Isend(
          msg, msg_size, MPI_BYTE, l2, 0, MPI_COMM_WORLD, mpi_event2.get_request()
        );
        parent_event.add_event(event_id2);
      }

      return parent_event_id;
    }
  }

  template <typename MessageT>
  event_t
  broadcast_msg(
    handler_t const& han, MessageT* const msg, action_t next_action = nullptr
  ) {
    return send_msg(-1, han, msg, next_action);
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

      if (msg->env.broadcast_root != -1) {
        send_msg_direct(-1, handler, msg, num_probe_bytes, [=]{
          delete [] buf;
        });
      }

      //std::cout << "scheduler: handler=" << handler << std::endl;

      auto active_fun = the_registry->get_handler(handler);
      active_fun(msg);

      if (msg->env.broadcast_root == -1) {
        delete [] buf;
      }

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
