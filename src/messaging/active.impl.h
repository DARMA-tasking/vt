
#if !defined INCLUDED_MESSAGING_ACTIVE_IMPL_H
#define INCLUDED_MESSAGING_ACTIVE_IMPL_H

#include "config.h"
#include "messaging/active.h"
#include "termination/term_headers.h"

namespace vt {

template <typename MessageT>
EventType ActiveMessenger::basicSendData(
  NodeType const& dest, MessageT* const msg, int const& msg_size,
  bool const& is_shared, bool const& is_term, EpochType const& epoch,
  TagType const& send_tag, EventRecordType* parent_event, ActionType next_action
) {
  auto const& this_node = theContext()->getNode();

  auto const event_id = theEvent()->createMPIEvent(this_node);
  auto& holder = theEvent()->getEventHolder(event_id);
  auto mpi_event = holder.get_event();

  if (is_shared) {
    mpi_event->setManagedMessage(msg);
  }

  if (not is_term) {
    theTerm()->produce(epoch);
  }

  MPI_Isend(
    msg, msg_size, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  if (parent_event) {
    parent_event->addEventToList(event_id);
  } else {
    holder.attachAction(next_action);
  }

  if (is_shared) {
    messageDeref(msg);
  }

  return event_id;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ACTIVE_IMPL_H*/
