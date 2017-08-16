
#include "common.h"
#include "active.h"
#include "transport.h"

namespace runtime {

event_t
ActiveMessenger::send_msg_direct(
  node_t const& dest, handler_t const& han, BaseMessage* const msg_base,
  int const& msg_size, action_t next_action
) {
  auto const& this_node = the_context->get_node();

  DEBUG_PRINT(
    "send_msg_direct: dest=%d, han=%d, msg=%p\n",
    dest, han, msg
  );

  auto msg = reinterpret_cast<ShortMessage* const>(msg_base);

  auto const& is_bcast = envelope_is_bcast(msg->env);
  auto const& is_term = envelope_is_term(msg->env);
  auto const& epoch =
    envelope_is_epoch_type(msg->env) ? envelope_get_epoch(msg->env) : no_epoch;

  if (not is_bcast) {
    // non-broadcast message send

    auto const event_id = the_event->create_mpi_event_id(this_node);
    auto& holder = the_event->get_event_holder(event_id);
    MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());

    if (not is_term) {
      the_term->produce(epoch);
    }

    MPI_Isend(
      msg, msg_size, MPI_BYTE, dest, 0, MPI_COMM_WORLD, mpi_event.get_request()
    );

    if (next_action != nullptr) {
      holder.attach_action(next_action);
    }

    return event_id;
  } else {
    // broadcast message send

    auto const& child1 = (this_node - dest)*2+1;
    auto const& child2 = (this_node - dest)*2+2;

    auto const& num_nodes = the_context->get_num_nodes();

    if (child1 >= num_nodes && child2 >= num_nodes) {
      if (next_action != nullptr) {
        next_action();
      }
      return no_event;
    }

    DEBUG_PRINT(
      "broadcast_msg: child1=%d, child2=%d, broadcast_root=%d, num_nodes=%d\n",
      child1, child2, dest, num_nodes
    );

    auto const parent_event_id = the_event->create_parent_event_id(this_node);
    auto& parent_holder = the_event->get_event_holder(parent_event_id);
    ParentEvent& parent_event = *static_cast<ParentEvent*>(parent_holder.get_event());

    if (next_action != nullptr) {
      parent_holder.attach_action(next_action);
    }

    if (child1 < num_nodes) {
      auto const event_id1 = the_event->create_mpi_event_id(this_node);
      auto& holder1 = the_event->get_event_holder(event_id1);
      MPIEvent& mpi_event1 = *static_cast<MPIEvent*>(holder1.get_event());

      DEBUG_PRINT(
        "broadcast_msg: sending to child1=%d, child2=%d, broadcast_root=%d, "
        "event_id=%lld\n",
        child1, child2, dest, event_id1
      );

      if (not is_term) {
        the_term->produce(epoch);
      }

      MPI_Isend(
        msg, msg_size, MPI_BYTE, child1, 0, MPI_COMM_WORLD, mpi_event1.get_request()
      );

      parent_event.add_event(event_id1);
    }

    if (child2 < num_nodes) {
      auto const event_id2 = the_event->create_mpi_event_id(this_node);
      auto& holder2 = the_event->get_event_holder(event_id2);
      MPIEvent& mpi_event2 = *static_cast<MPIEvent*>(holder2.get_event());

      DEBUG_PRINT(
        "broadcast_msg: sending to child2=%d, child1=%d, broadcast_root=%d, "
        "event_id=%lld\n",
        child2, child1, dest, event_id2
      );

      if (not is_term) {
        the_term->produce(epoch);
      }

      MPI_Isend(
        msg, msg_size, MPI_BYTE, child2, 0, MPI_COMM_WORLD, mpi_event2.get_request()
      );

      parent_event.add_event(event_id2);
    }

    return parent_event_id;
  }
}

bool
ActiveMessenger::try_process_incoming_message() {
  byte_t num_probe_bytes;
  MPI_Status stat;
  int flag;

  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);

  if (flag == 1) {
    MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

    char* buf = new char[num_probe_bytes];

    MPI_Recv(
      buf, num_probe_bytes, MPI_BYTE, stat.MPI_SOURCE, stat.MPI_TAG,
      MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    Message* msg = reinterpret_cast<Message*>(buf);

    auto const& is_term = envelope_is_term(msg->env);
    auto const& is_bcast = envelope_is_bcast(msg->env);

    auto const& handler = envelope_get_handler(msg->env);
    auto const& epoch =
      envelope_is_epoch_type(msg->env) ? envelope_get_epoch(msg->env) : no_epoch;

    auto const& active_fun = the_registry->get_handler(handler);

    // run the active function
    active_fun(msg);

    // @todo: the broadcast forward should happen before running the active
    // function, but deallocation is a problem without reference counting
    if (is_bcast) {
      send_msg_direct(-1, handler, msg, num_probe_bytes, [=]{
        delete [] buf;
      });
    }

    // do not touch msg after this---unsafe may be deallocated

    DEBUG_PRINT(
      "%d: try_process_incoming_message: consume: msg=%p, is_term=%d\n",
      the_context->get_node(), msg, is_term
    );

    if (not is_term) {
      the_term->consume(epoch);
    }

    if (not is_bcast) {
      delete [] buf;
    }

    return true;
  } else {
    return false;
  }
}


void
ActiveMessenger::scheduler(int const& num_times) {
  for (int i = 0; i < num_times; i++) {
    try_process_incoming_message();
    perform_triggered_actions();
    //the_term->maybe_propagate();
  }
}

void
ActiveMessenger::perform_triggered_actions() {
  the_event->test_events_trigger(mpi_event_tag);
  the_event->test_events_trigger(normal_event_tag);
}


} //end namespace runtime
