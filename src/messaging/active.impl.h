
#if !defined INCLUDED_MESSAGING_ACTIVE_IMPL_H
#define INCLUDED_MESSAGING_ACTIVE_IMPL_H

#include "config.h"
#include "messaging/active.h"
#include "termination/term_headers.h"

namespace vt {

template <typename MessageT>
void ActiveMessenger::setTermMessage(MessageT* const msg) {
  setTermType(msg->env);
}

template <typename MessageT>
void ActiveMessenger::setEpochMessage(
  MessageT* const msg, EpochType const& epoch
) {
  envelopeSetEpoch(msg->env, epoch);
}

template <typename MessageT>
void ActiveMessenger::setTagMessage(MessageT* const msg, TagType const& tag) {
  envelopeSetTag(msg->env, tag);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void ActiveMessenger::trigger(std::function<void(vt::BaseMessage*)> fn) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
  theRegistry()->saveTrigger(han, /*reinterpret_cast<active_function_t>(*/fn);
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ACTIVE_IMPL_H*/
