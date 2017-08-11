
#if ! defined __RUNTIME_TRANSPORT_EVENT__
#define __RUNTIME_TRANSPORT_EVENT__

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <unordered_map>

#include "common.h"

namespace runtime {

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

std::unique_ptr<AsyncEvent> the_event = std::make_unique<AsyncEvent>();

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT__*/
