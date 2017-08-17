
#if ! defined __RUNTIME_TRANSPORT_EVENT__
#define __RUNTIME_TRANSPORT_EVENT__

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <unordered_map>

#include "common.h"
#include "event_msgs.h"
#include "context.h"

namespace runtime {

struct Event {
  event_t event_id = 0;
  int event_tag = 0;

  Event(event_t const& in_event_id, int const& event_tag_in)
    : event_id(in_event_id), event_tag(event_tag_in)
  { }

  virtual bool test_ready() = 0;
  virtual bool wait_ready() = 0;
  virtual void set_ready() = 0;
};

struct ParentEvent : Event {
  virtual void set_ready() {
    assert(0);
  }

  virtual bool test_ready();

  virtual bool wait_ready() {
    assert(0);
    return true;
  }

  void add_event(event_t event) {
    events.push_back(event);
  }

  ParentEvent(event_t const& in_event_id)
    : Event(in_event_id, normal_event_tag)
  { }

private:
  std::vector<event_t> events;
};

struct NormalEvent : Event {
  virtual void set_ready() {
    complete = true;
  }

  virtual bool test_ready() {
    return complete;
  }

  virtual bool wait_ready() {
    while (!test_ready()) {
      assert(0);
      // TODO: call into scheduler
    }
    return true;
  }

  NormalEvent(event_t const& in_event_id)
    : Event(in_event_id, normal_event_tag)
  { }

private:
  bool complete = false;
};

struct MPIEvent : Event {
  virtual void set_ready() { assert(0); }

  virtual bool test_ready() {
    //printf("MPIEvent test_ready() id=%lld\n", event_id);
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
    : Event(in_event_id, mpi_event_tag)
  { }

private:
  MPI_Status stat;
  MPI_Request req;
  int flag = 0;
};

enum class EventState : int8_t {
  EventReady = 1,
  EventWaiting = 2,
  EventRemote = 3
};

struct AsyncEvent {
  using event_state_t = EventState;
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
    make_ready_trigger();

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
  using typed_event_container_t = std::list<event_holder_ptr_t>;

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

  node_t
  get_owning_node(event_t const& event) {
    node_t const node = event >> (64 - (sizeof(node_t) * 8));
    return node;
  }

  event_t
  create_mpi_event_id(node_t const& node) {
    auto const& evt = create_event_id<MPIEvent>(node);
    auto& holder = get_event_holder(evt);
    event_container[mpi_event_tag].emplace_back(
      &holder
    );
    return evt;
  }

  event_t
  create_normal_event_id(node_t const& node) {
    return create_event_id<NormalEvent>(node);
  }

  event_t
  create_parent_event_id(node_t const& node) {
    auto const& evt = create_event_id<ParentEvent>(node);
    auto& holder = get_event_holder(evt);
    event_container[mpi_event_tag].emplace_back(
      &holder
    );
    return evt;
  }

  event_holder_t&
  get_event_holder(event_t const& event) {
    auto const& owning_node = get_owning_node(event);

    debug_print_event(
      "the_event: get_event_holder: node=%d, event=%lld, owning_node=%d\n",
      the_context->get_node(), event, owning_node
    );

    if (owning_node != the_context->get_node()) {
      assert(
        0 && "Event does not belong to this node"
      );
    }

    auto container_iter = container.find(event);
    assert(
      container_iter != container.end() and "Event must exist in container"
    );
    return container_iter->second;
  }

  bool
  holder_exists(event_t const& event) {
    auto container_iter = container.find(event);
    return container_iter != container.end();
  }

  template <typename EventT>
  EventT&
  get_event(event_t const& event) {
    return *static_cast<EventT*>(get_event_holder(event).get_event());
  }

  event_state_t
  test_event_complete(event_t const& event) {
    if (holder_exists(event)) {
      bool const is_ready = this->get_event_holder(event).get_event()->test_ready();
      if (is_ready) {
        return event_state_t::EventReady;
      } else {
        return event_state_t::EventWaiting;
      }
    } else {
      if (get_owning_node(event) == the_context->get_node()) {
        return event_state_t::EventReady;
      } else {
        return event_state_t::EventRemote;
      }
    }
  }

  event_t
  attach_action(event_t const& event, action_t callable);

  void
  test_events_trigger(
    int const& event_tag, int const& num_events = num_check_actions
  ) {
    int cur = 0;
    for (auto iter = event_container[event_tag].begin();
         cur < num_events && iter != event_container[event_tag].end();
         ++iter) {
      auto const& holder = *iter;
      auto event = holder->get_event();
      if (event->test_ready()) {
        holder->execute_actions();
        iter = event_container[event_tag].erase(iter);
        container.erase(event->event_id);
        return;
      }
      cur++;
    }
  }

  static void
  register_event_handlers();

  handler_t event_finished_han = uninitialized_handler;
  handler_t check_event_finished_han = uninitialized_handler;

private:
  event_t cur_event = 0;

  typed_event_container_t event_container[2];

  event_container_t container;
};

extern std::unique_ptr<AsyncEvent> the_event;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT__*/
