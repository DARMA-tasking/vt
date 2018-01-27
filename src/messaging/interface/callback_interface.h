
#if !defined INCLUDED_MESSAGING_INTERFACE_CALLBACK_INTERFACE_H
#define INCLUDED_MESSAGING_INTERFACE_CALLBACK_INTERFACE_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"

namespace vt { namespace messaging { namespace interface {

struct ActiveCallbackFn {
  using HandlerManagerType = HandlerManager;

   /*----------------------------------------------------------------------------
   *                            Send Message Callback
   *----------------------------------------------------------------------------
   *
   * Send message *callback* variants (automatically allow user to callback upon
   * message arrival)
   *
   *----------------------------------------------------------------------------
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendDataCallback(
    NodeType const& dest, MessageT* const msg, ActiveClosureFnType fn
  );

  template <typename MessageT>
  void sendDataCallback(
    HandlerType const& han, NodeType const& dest, MessageT* const msg,
    ActiveClosureFnType fn
  );

  template <typename MessageT>
  void sendCallback(MessageT* const msg);

  /*
   *----------------------------------------------------------------------------
   *                        End Send Message Callback
   *----------------------------------------------------------------------------
   */

};

}}} /* end namespace vt::messaging::interface */

#endif /*INCLUDED_MESSAGING_INTERFACE_CALLBACK_INTERFACE_H*/
