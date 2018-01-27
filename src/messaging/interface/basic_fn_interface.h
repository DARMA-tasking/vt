
#if !defined INCLUDED_MESSAGING_INTERFACE_BASIC_FN_INTERFACE_H
#define INCLUDED_MESSAGING_INTERFACE_BASIC_FN_INTERFACE_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"

namespace vt { namespace messaging { namespace interface {

struct ActiveBasicFn {
  using HandlerManagerType = HandlerManager;

  /*----------------------------------------------------------------------------
   *                 Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   *
   * Send message using basic function handler. These handlers are NOT type-safe
   * and require the user to cast their message to the correct type as so:
   *
   *   void basicHandler(vt::BaseMessage* msg_in) {
   *     MyMsg* msg = static_cast<MyMsg*>(msg_in);
   *     ...
   *   }
   *
   *  theMsg()->sendMsg<basicHandler, MyMsg>(1, msg);
   *
   * Most likely this will be deprecated unless there is a use for this, since
   * type safety does not cost anything in terms of overhead (either at runtime
   * or compile-time).
   *
   *----------------------------------------------------------------------------
   */

  template <ActiveFnType* f, typename MessageT>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <ActiveFnType* f, typename MessageT>
  EventType broadcastMsg(MessageT* const msg, ActionType act);

  template <ActiveFnType* f, typename MessageT>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <ActiveFnType* f, typename MessageT>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act);

  /*
   *----------------------------------------------------------------------------
   *              End Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   */
};

}}} /* end namespace vt::messaging::interface */

#endif /*INCLUDED_MESSAGING_INTERFACE_BASIC_FN_INTERFACE_H*/
