
#if !defined INCLUDED_MESSAGING_INTERFACE_PREREGISTER_INTERFACE_H
#define INCLUDED_MESSAGING_INTERFACE_PREREGISTER_INTERFACE_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"

namespace vt { namespace messaging { namespace interface {

struct ActivePreregisterFn {
  using HandlerManagerType = HandlerManager;

  /*----------------------------------------------------------------------------
   *            Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   *
   * Send message  to pre-registered active message handler.
   *
   *   void myHandler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *   HandlerType const han = registerNewHandler(my_handler);
   *
   *   MyMsg* msg = makeSharedMessage<MyMsg>(156);
   *   theMsg()->sendMsg(29, han, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT>
  EventType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    ActionType next_action = nullptr
  );

  template <typename MessageT>
  EventType sendMsg(
    HandlerType const& han, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  /*
   *----------------------------------------------------------------------------
   *          End Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   */
};

}}} /* end namespace vt::messaging::interface */

#endif /*INCLUDED_MESSAGING_INTERFACE_PREREGISTER_INTERFACE_H*/
