
#if !defined INCLUDED_MESSAGING_INTERFACE_AUTO_FN_INTERFACE_H
#define INCLUDED_MESSAGING_INTERFACE_AUTO_FN_INTERFACE_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"
#include "handler/handler.h"

namespace vt { namespace messaging { namespace interface {

struct ActiveAutoFn {
  using HandlerManagerType = HandlerManager;

  /*----------------------------------------------------------------------------
   *              Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   *
   * Send message using a type-safe function handler. This is the predominant
   * way that the messenger is expected to be used.
   *
   *   void myHandler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *  theMsg()->sendMsg<MyMsg, myHandler>(1, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType broadcastMsg(MessageT* const msg, ActionType act);

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act);

  /*
   *----------------------------------------------------------------------------
   *             End Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   */
};

}}} /* end namespace vt::messaging::interface */

#endif /*INCLUDED_MESSAGING_INTERFACE_AUTO_FN_INTERFACE_H*/
