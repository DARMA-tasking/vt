
#if ! defined __RUNTIME_TRANSPORT__
#define __RUNTIME_TRANSPORT__

#include <cstdint>
#include <cassert>
#include <memory>
#include <vector>
#include <functional>
#include <list>
#include <iostream>
#include <unordered_map>
#include <mpi.h>

namespace runtime {

using node_t = int16_t;
using handler_t = int32_t;
using envelope_datatype_t = int8_t;
using event_t = uint64_t;

enum class EnvelopeType : envelope_datatype_t {
  Normal,
  Get,
  Put
};

using envelope_type_t = EnvelopeType;

struct Envelope {
  node_t dest;
  handler_t han;
  envelope_type_t type;

  Envelope() = default;

  Envelope(
    node_t const& in_dest, handler_t const& in_han,
    envelope_type_t const& in_type
  ) : dest(in_dest), han(in_han), type(in_type)
  { }
};

struct Message {
  using envelope_t = Envelope;

  envelope_t env;

  Message() = default;
};

using active_function_t = std::function<void(Message*)>;
using action_t = std::function<void()>;

constexpr int const num_check_actions = 8;
constexpr int const scheduler_default_num_times = 1;

// template <typename T>
// struct Message {
//   using envelope_t = Envelope;
//   using user_type_t = T;

//   envelope_t env;
//   user_type_t user_data;

//   Message() = default;

//   Message(
//     envelope_t&& in_env, user_type_t&& in_user_data
//   ) : env(std::move(in_env)), user_data(std::move(in_user_data))
//   { }
// };

struct Event {
  event_t event_id;

  Event(event_t const& in_event_id)
    : event_id(in_event_id)
  { }

  virtual bool test_ready() = 0;
  virtual bool wait_ready() = 0;
};

struct MPIEvent : Event {

  virtual bool test_ready() {
    MPI_Test(&req, &flag, &stat);
    return flag == 1;
  }

  virtual bool wait_ready() {
    while (!test_ready()) ;
    return true;
  }

  MPI_Request* get_request() {
    return &req;
  }

  MPIEvent(event_t const& in_event_id)
    : Event(in_event_id)
  { }

private:
  MPI_Status stat;
  MPI_Request req;
  int flag = 0;
};

struct AsyncEvent {
  using event_wrapper_t = Event;
  using event_wrapper_ptr_t = std::unique_ptr<event_wrapper_t>;

  struct EventHolder {
    using action_container_t = std::vector<action_t>;

    EventHolder(event_wrapper_ptr_t in_event)
      : event(std::move(in_event))
    { }

    event_wrapper_t*
    get_event() const {
      return event.get();
    }

    void
    attach_action(action_t action) {
      actions.emplace_back(action);
    }

    void
    execute_actions() {
      for (auto&& action : actions) {
        action();
      }
      actions.clear();
    }

  private:
    event_wrapper_ptr_t event = nullptr;
    action_container_t actions;
  };

  using event_holder_t = EventHolder;
  using event_holder_ptr_t = EventHolder*;
  using event_container_t = std::unordered_map<event_t, event_holder_t>;
  using mpi_event_container_t = std::list<event_holder_ptr_t>;

  AsyncEvent() = default;

  template <typename EventT>
  event_t
  create_event_id(node_t const& node) {
    event_t const event = (event_t)node << (64 - (sizeof(node_t) * 8)) | cur_event;
    cur_event++;
    std::unique_ptr<EventT> et = std::make_unique<EventT>(event);
    container.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(event),
      std::forward_as_tuple(event_holder_t(std::move(et)))
    );
    return event;
  }

  event_t
  create_mpi_event_id(node_t const& node) {
    auto const& evt = create_event_id<MPIEvent>(node);
    auto& holder = get_event_holder(evt);
    mpi_event_container.emplace_back(
      &holder
    );
    return evt;
  }

  event_holder_t&
  get_event_holder(event_t const& event) {
    auto container_iter = container.find(event);
    assert(
      container_iter != container.end() and "Event must exist in container"
    );
    return container_iter->second;
  }

  template <typename EventT>
  EventT&
  get_event(event_t const& event) {
    return *static_cast<EventT*>(get_event_holder(event).get_event());
  }

  void
  attach_action(event_t const& event, action_t callable) {
    this->get_event_holder(event).attach_action(
      callable
    );
  }

  void
  test_mpi_events_trigger(int const& num_events = num_check_actions) {
    int cur = 0;
    for (auto iter = mpi_event_container.begin();
         cur < num_events && iter != mpi_event_container.end();
         ++iter) {
      auto const& holder = *iter;
      auto event = holder->get_event();
      if (event->test_ready()) {
        holder->execute_actions();
        iter = mpi_event_container.erase(iter);
        container.erase(event->event_id);
        return;
      }
      cur++;
    }
  }

private:
  event_t cur_event = 0;

  mpi_event_container_t mpi_event_container;

  event_container_t container;
};

struct Registry {
  using container_t = std::vector<active_function_t>;
  using register_count_t = uint32_t;

  Registry() = default;

  handler_t
  register_active_handler(active_function_t fn) {
    registered.resize(cur_empty_slot+1);
    registered.at(cur_empty_slot) = fn;
    cur_empty_slot++;
    return cur_empty_slot-1;
  }

  active_function_t
  get_handler(handler_t const& han) {
    assert(
      registered.size()-1 >= han and "Handler must be registered"
    );
    return registered.at(han);
  }

private:
  container_t registered;

  register_count_t cur_empty_slot = 0;
};

struct Context {
  Context(
    node_t const& in_this_node, node_t const& in_num_nodes
  ) : this_node(in_this_node), num_nodes(in_num_nodes)
  { }

  node_t get_node() const { return this_node; }
  node_t get_num_nodes() const { return num_nodes; }

private:
  node_t this_node = 0;
  node_t num_nodes = 0;
};

std::unique_ptr<Registry> the_registry = std::make_unique<Registry>();
std::unique_ptr<AsyncEvent> the_event = std::make_unique<AsyncEvent>();
std::unique_ptr<Context> the_context = nullptr;

struct CollectiveOps {
  static void
  initialize_context(int argc, char** argv) {
    int num_nodes = 0, this_node = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &this_node);
    the_context = std::make_unique<Context>(this_node, num_nodes);
  }

  static handler_t
  register_handler(active_function_t fn) {
    return the_registry->register_active_handler(
      static_cast<active_function_t>(fn)
    );
  }

  static void
  finalize_context() {
    MPI_Barrier(MPI_COMM_WORLD);
    the_context = nullptr;
    MPI_Finalize();
  }
};

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
    node_t this_node = 0;
    //auto const event_id = the_event->template create_event_id<MPIEvent>(this_node);
    auto const event_id = the_event->create_mpi_event_id(this_node);
    auto& holder = the_event->get_event_holder(event_id);
    MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());
    std::cout << "ActiveMessenger: sending, handler=" << han << ", "
              << "dest=" << dest << ","
              << "size=" << msg_size << ","
              << std::endl;
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
    the_event->test_mpi_events_trigger();
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
      std::cout << "scheduler: handler=" << handler << std::endl;
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

std::unique_ptr<ActiveMessenger> the_msg = std::make_unique<ActiveMessenger>();

}

#endif /*__RUNTIME_TRANSPORT__*/
