
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

  virtual bool testReady() = 0;
  virtual bool waitReady() = 0;
  virtual void setReady() = 0;
};

struct ParentEvent : Event {
  virtual void setReady() {
    assert(0);
  }

  virtual bool testReady();

  virtual bool waitReady() {
    assert(0);
    return true;
  }

  void add_event(EventType event) {
    events.push_back(event);
  }

  explicit ParentEvent(EventType const& in_event_id)
    : Event(in_event_id, normal_event_tag)
  { }

private:
  std::vector<EventType> events;
};

struct NormalEvent : Event {
  virtual void setReady() {
    complete = true;
  }

  virtual bool testReady() {
    return complete;
  }

  virtual bool waitReady() {
    while (!testReady()) {
      assert(0);
      // TODO: call into scheduler
    }
    return true;
  }

  explicit NormalEvent(EventType const& in_event_id)
    : Event(in_event_id, normal_event_tag)
  { }

private:
  bool complete = false;
};

struct MPIEvent : Event {
  virtual void setReady() { assert(0); }

  virtual bool testReady() {
    //printf("MPIEvent testReady() id=%lld\n", event_id);
    MPI_Test(&req, &flag, &stat);
    bool const ready = flag == 1;

    if (ready and msg != nullptr and isSharedMessage(msg)) {
      messageDeref(msg);
      msg = nullptr;
    }

    return ready;
  }

  virtual bool waitReady() {
    while (!testReady()) ;
    return true;
  }

  MPI_Request* getRequest() {
    return &req;
  }

  MPIEvent(EventType const& in_event_id, ShortMessage* in_msg = nullptr)
    : Event(in_event_id, mpi_event_tag), msg(in_msg)
  { }

  void setManagedMessage(ShortMessage* in_msg) {
    msg = in_msg;
    messageRef(msg);
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

    explicit EventHolder(EventWrapperPtrType in_event)
      : event(std::move(in_event))
    { }

    EventWrapperType* get_event() const {
      return event.get();
    }

    void attachAction(ActionType action) {
      actions.emplace_back(action);
    }

    void makeReadyTrigger();

    void executeActions() {
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
  EventType createEventId(NodeType const& node) {
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

  NodeType getOwningNode(EventType const& event) {
    NodeType const node = event >> (64 - (sizeof(NodeType) * 8));
    return node;
  }

  EventType createMpiEventId(NodeType const& node) {
    auto const& evt = createEventId<MPIEvent>(node);
    auto& holder = getEventHolder(evt);
    event_container_[mpi_event_tag].emplace_back(
      &holder
    );
    return evt;
  }

  EventType createNormalEventId(NodeType const& node) {
    return createEventId<NormalEvent>(node);
  }

  EventType createParentEventId(NodeType const& node) {
    auto const& evt = createEventId<ParentEvent>(node);
    auto& holder = getEventHolder(evt);
    event_container_[mpi_event_tag].emplace_back(
      &holder
    );
    return evt;
  }

  EventHolderType& getEventHolder(EventType const& event) {
    auto const& owning_node = getOwningNode(event);

    debug_print(
      event, node,
      "theEvent: get_event_holder: node=%d, event=%lld, owning_node=%d\n",
      theContext->getNode(), event, owning_node
    );

    if (owning_node != theContext->getNode()) {
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

  bool holderExists(EventType const& event) {
    auto container_iter = container_.find(event);
    return container_iter != container_.end();
  }

  template <typename EventT>
  EventT& getEvent(EventType const& event) {
    return *static_cast<EventT*>(getEventHolder(event).get_event());
  }

  EventStateType testEventComplete(EventType const& event) {
    if (holderExists(event)) {
      bool const is_ready = this->getEventHolder(event).get_event()->testReady();
      if (is_ready) {
        return EventStateType::EventReady;
      } else {
        return EventStateType::EventWaiting;
      }
    } else {
      if (getOwningNode(event) == theContext->getNode()) {
        return EventStateType::EventReady;
      } else {
        return EventStateType::EventRemote;
      }
    }
  }

  EventType attachAction(EventType const& event, ActionType callable);

  void testEventsTrigger(
    int const& event_tag, int const& num_events = num_check_actions
  ) {
    int cur = 0;
    for (auto iter = event_container_[event_tag].begin();
         cur < num_events && iter != event_container_[event_tag].end();
         ++iter) {
      auto const& holder = *iter;
      auto event = holder->get_event();
      if (event->testReady()) {
        holder->executeActions();
        iter = event_container_[event_tag].erase(iter);
        container_.erase(event->event_id);
        return;
      }
      cur++;
    }
  }

  bool scheduler();

  bool isLocalTerm();

  static void eventFinished(EventFinishedMsg* msg);
  static void checkEventFinished(EventCheckFinishedMsg* msg);

private:
  EventType cur_event_ = 0;

  TypedEventContainerType event_container_[2];

  EventContainerType container_;
};

extern std::unique_ptr<AsyncEvent> theEvent;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_EVENT__*/
