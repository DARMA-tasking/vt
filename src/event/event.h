
#if ! defined __RUNTIME_TRANSPORT_EVENT__
#define __RUNTIME_TRANSPORT_EVENT__

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <unordered_map>

#include <mpi.h>

#include "common.h"
#include "event_msgs.h"
#include "context.h"

namespace vt {

struct Event {
  EventType event_id = 0;
  int event_tag = 0;

  Event(EventType const& in_event_id, int const& event_tag_in)
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

  void add_event(EventType event) {
    events.push_back(event);
  }

  ParentEvent(EventType const& in_event_id)
    : Event(in_event_id, normal_event_tag)
  { }

private:
  std::vector<EventType> events;
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

  NormalEvent(EventType const& in_event_id)
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
    bool const ready = flag == 1;

    if (ready and msg != nullptr and is_shared_message(msg)) {
      message_deref(msg);
      msg = nullptr;
    }

    return ready;
  }

  virtual bool wait_ready() {
    while (!test_ready()) ;
    return true;
  }

  MPI_Request* get_request() {
    return &req;
  }

  MPIEvent(EventType const& in_event_id, ShortMessage* in_msg = nullptr)
    : Event(in_event_id, mpi_event_tag), msg(in_msg)
  { }

  void set_managed_message(ShortMessage* in_msg) {
    msg = in_msg;
    message_ref(msg);
  }

private:
  ShortMessage* msg = nullptr;
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
  using EventStateType = EventState;
  using EventWrapperType = Event;
  using EventWrapperPtrType = std::unique_ptr<EventWrapperType>;

  struct EventHolder {
    using ActionContainerType = std::vector<ActionType>;

    EventHolder(EventWrapperPtrType in_event)
      : event(std::move(in_event))
    { }

    EventWrapperType* get_event() const {
      return event.get();
    }

    void attach_action(ActionType action) {
      actions.emplace_back(action);
    }

    void make_ready_trigger();

    void execute_actions() {
      for (auto&& action : actions) {
        action();
      }
      actions.clear();
    }

  private:
    EventWrapperPtrType event = nullptr;
    ActionContainerType actions;
  };

  using EventHolderType = EventHolder;
  using EventHolderPtrType = EventHolder*;
  using EventContainerType = std::unordered_map<EventType, EventHolderType>;
  using TypedEventContainerType = std::list<EventHolderPtrType>;

  AsyncEvent() = default;

  template <typename EventT>
  EventType create_event_id(NodeType const& node) {
    EventType const event =
      (EventType)node << (64 - (sizeof(NodeType) * 8)) | cur_event_;
    cur_event_++;
    std::unique_ptr<EventT> et = std::make_unique<EventT>(event);
    container_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(event),
      std::forward_as_tuple(EventHolderType(std::move(et)))
    );
    return event;
  }

  NodeType get_owning_node(EventType const& event) {
    NodeType const node = event >> (64 - (sizeof(NodeType) * 8));
    return node;
  }

  EventType create_mpi_event_id(NodeType const& node) {
    auto const& evt = create_event_id<MPIEvent>(node);
    auto& holder = get_event_holder(evt);
    event_container_[mpi_event_tag].emplace_back(
      &holder
    );
    return evt;
  }

  EventType create_normal_event_id(NodeType const& node) {
    return create_event_id<NormalEvent>(node);
  }

  EventType create_parent_event_id(NodeType const& node) {
    auto const& evt = create_event_id<ParentEvent>(node);
    auto& holder = get_event_holder(evt);
    event_container_[mpi_event_tag].emplace_back(
      &holder
    );
    return evt;
  }

  EventHolderType& get_event_holder(EventType const& event) {
    auto const& owning_node = get_owning_node(event);

    debug_print(
      event, node,
      "the_event: get_event_holder: node=%d, event=%lld, owning_node=%d\n",
      the_context->get_node(), event, owning_node
    );

    if (owning_node != the_context->get_node()) {
      assert(
        0 && "Event does not belong to this node"
      );
    }

    auto container_iter = container_.find(event);
    assert(
      container_iter != container_.end() and "Event must exist in container"
    );
    return container_iter->second;
  }

  bool holder_exists(EventType const& event) {
    auto container_iter = container_.find(event);
    return container_iter != container_.end();
  }

  template <typename EventT>
  EventT& get_event(EventType const& event) {
    return *static_cast<EventT*>(get_event_holder(event).get_event());
  }

  EventStateType test_event_complete(EventType const& event) {
    if (holder_exists(event)) {
      bool const is_ready = this->get_event_holder(event).get_event()->test_ready();
      if (is_ready) {
        return EventStateType::EventReady;
      } else {
        return EventStateType::EventWaiting;
      }
    } else {
      if (get_owning_node(event) == the_context->get_node()) {
        return EventStateType::EventReady;
      } else {
        return EventStateType::EventRemote;
      }
    }
  }

  EventType attach_action(EventType const& event, ActionType callable);

  void test_events_trigger(
    int const& event_tag, int const& num_events = num_check_actions
  ) {
    int cur = 0;
    for (auto iter = event_container_[event_tag].begin();
         cur < num_events && iter != event_container_[event_tag].end();
         ++iter) {
      auto const& holder = *iter;
      auto event = holder->get_event();
      if (event->test_ready()) {
        holder->execute_actions();
        iter = event_container_[event_tag].erase(iter);
        container_.erase(event->event_id);
        return;
      }
      cur++;
    }
  }

  bool scheduler();

  bool is_local_term();

  static void event_finished(EventFinishedMsg* msg);
  static void check_event_finished(EventCheckFinishedMsg* msg);

private:
  EventType cur_event_ = 0;

  TypedEventContainerType event_container_[2];

  EventContainerType container_;
};

extern std::unique_ptr<AsyncEvent> the_event;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_EVENT__*/
