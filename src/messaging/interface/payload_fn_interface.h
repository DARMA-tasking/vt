
#if !defined INCLUDED_MESSAGING_INTERFACE_PAYLOAD_FN_INTERFACE_H
#define INCLUDED_MESSAGING_INTERFACE_PAYLOAD_FN_INTERFACE_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"
#include "event/event.h"

#include <functional>
#include <tuple>

namespace vt { namespace messaging { namespace interface {

struct ActivePayloadFn {
  using SendDataRetType = std::tuple<EventType, TagType>;
  using SendFnType = std::function<
    SendDataRetType(RDMA_GetType,NodeType,TagType,ActionType)
  >;
  using UserSendFnType = std::function<void(SendFnType)>;
  using HandlerManagerType = HandlerManager;

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
  EventType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    UserSendFnType send_payload_fn, ActionType next_action = nullptr
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn,
    ActionType next_action = nullptr
  );

  template <typename MessageT>
  EventType broadcastMsg(
    HandlerType const& han, MessageT* const msg, ActionType next_action = nullptr
  );
  /*
   *----------------------------------------------------------------------------
   *                           End Send Data Message
   *----------------------------------------------------------------------------
   */
};

}}} /* end namespace vt::messaging::interface */

#endif /*INCLUDED_MESSAGING_INTERFACE_PAYLOAD_FN_INTERFACE_H*/
