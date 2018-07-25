
#if !defined INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_IMPL_H
#define INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal_holder.h"
#include "pipe/callback/callback_base.h"

#include <cassert>
#include <unordered_map>
#include <vector>
#include <memory>

namespace vt { namespace pipe { namespace signal {

template <typename SignalT>
void SignalHolder<SignalT>::addSignal(PipeType const& pid, DataType in_data) {
  auto iter = pending_holder_.find(pid);
  if (iter == pending_holder_.end()) {
    pending_holder_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(pid),
      std::forward_as_tuple(SignalListType{in_data})
    );
  } else {
    iter->second.push_back(in_data);
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::removeListener(PipeType const& pid, ListenerPtrType listener) {
  auto iter = listeners_.find(pid);
  if (iter != listeners_.end()) {
    auto liter = iter->second.begin();
    while (liter != iter->second.end() && liter.get() != listener) ++liter;
    if (liter != iter->second.end()) {
      ListenerType elm = std::move(*liter);
      iter->second.erase(liter);
      elm = nullptr;
    }
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::clearAllListeners(PipeType const& pid) {
  auto iter = listeners_.find(pid);
  if (iter != listeners_.end()) {
    iter->second.clear();
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::clearAllSignals(PipeType const& pid) {
  auto iter = pending_holder_.find(pid);
  if (iter != pending_holder_.end()) {
    iter->second.clear();
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::clearPipe(PipeType const& pid) {
  clearAllListeners(pid);
  clearAllSignals(pid);
}

template <typename SignalT>
SignalHolder<SignalT>::ListenerListIterType
SignalHolder<SignalT>::removeListener(
  ListenerListIterType iter
) {
  ListenerType elm = std::move(*iter);
  auto niter = iter->second.erase(iter);
  // explicitly set to nullptr to invoke destructor (instead of waiting for
  // out of scope)
  elm = nullptr;
  return niter;
}

template <typename SignalT>
void SignalHolder<SignalT>::deliverAll(PipeType const& pid, DataType data) {
  auto iter = pending_holder_.find(pid);
  if (iter != pending_holder_.end()) {
    auto liter = iter->second.begin();
    while (liter != iter->second.end()) {
      applySignal(liter->get(), data);
      auto const& is_fin = finished(liter->get());
      if (is_fin) {
        liter = removeListener(liter);
      } else {
        liter++;
      }
    }
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::addListener(PipeType const& pid, ListenerType&& cb) {
  auto iter = listeners_.find(pid);
  if (iter == listeners_.end()) {
    listeners_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(pid),
      std::forward_as_tuple(ListenerListType{std::move(cb)})
    );
  } else {
    iter->second.emplace_back(std::move(cb));
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::applySignal(
  ListenerPtrType listener, DataType data
) {
  listener->trigger(data);
}

template <typename SignalT>
bool SignalHolder<SignalT>::finished(ListenerPtrType listener) const {
  return listener->finished();
}

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_IMPL_H*/
