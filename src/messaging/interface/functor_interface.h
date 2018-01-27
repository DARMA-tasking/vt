
#if !defined INCLUDED_MESSAGING_INTERFACE_FUNCTOR_INTERFACE_H
#define INCLUDED_MESSAGING_INTERFACE_FUNCTOR_INTERFACE_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"

namespace vt { namespace messaging { namespace interface {

struct ActiveFunctorFn {
  using HandlerManagerType = HandlerManager;

  /*----------------------------------------------------------------------------
   *                       Send Message Functor Variants
   *----------------------------------------------------------------------------
   *
   * Send message Functor variants that cause an active message to trigger a
   * user-defined functor such as:
   *
   *   struct X {
   *     void operator()(MyMsg* msg) const { ... };
   *   };
   *
   *----------------------------------------------------------------------------
   */

  template <typename FunctorT, typename MessageT>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename FunctorT, typename MessageT>
  EventType broadcastMsg(MessageT* const msg, ActionType act);

  template <typename FunctorT, typename MessageT>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename FunctorT, typename MessageT>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act);

  /*
   *----------------------------------------------------------------------------
   *                      End Send Message Functor Variants
   *----------------------------------------------------------------------------
   */
};

}}} /* end namespace vt::messaging::interface */

#endif /*INCLUDED_MESSAGING_INTERFACE_FUNCTOR_INTERFACE_H*/
