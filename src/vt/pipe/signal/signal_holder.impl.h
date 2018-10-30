
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
void SignalHolder<SignalT>::addSignal(
  PipeType const& pid, DataPtrType in_data
) {
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
void SignalHolder<SignalT>::removeListener(
  PipeType const& pid, ListenerPtrType listener
) {
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
typename SignalHolder<SignalT>::ListenerListIterType
SignalHolder<SignalT>::removeListener(
  ListenerMapIterType map_iter, ListenerListIterType iter
) {
  ListenerType elm = std::move(*iter);
  auto niter = map_iter->second.erase(iter);
  // explicitly set to nullptr to invoke destructor (instead of waiting for
  // out of scope)
  elm = nullptr;
  return niter;
}

template <typename SignalT>
typename SignalHolder<SignalT>::SigCountType
SignalHolder<SignalT>::getCount(PipeType const& pid) {
  auto iter = listener_count_.find(pid);
  if (iter != listener_count_.end()) {
    return iter->second;
  } else {
    return -1;
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::setCount(
  PipeType const& pid, SigCountType const& count
) {
  auto iter = listener_count_.find(pid);
  if (iter != listener_count_.end()) {
    iter->second = count;
  } else {
    listener_count_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(pid),
      std::forward_as_tuple(count)
    );
  }
}

template <typename SignalT>
void SignalHolder<SignalT>::deliverAll(PipeType const& pid, DataPtrType data) {
  auto const& count = getCount(pid);
  auto iter = listeners_.find(pid);
  if (iter != listeners_.end()) {
    auto liter = iter->second.begin();
    while (liter != iter->second.end()) {
      applySignal(liter->get(),data,pid);
      auto const& is_fin = finished(liter->get());
      if (is_fin) {
        liter = removeListener(iter,liter);
      } else {
        liter++;
      }
    }
    if (iter->second.size() == 0) {
      listeners_.erase(iter);
    }
  } else {
    assert(0 && "At least one listener should exist");
  }
  // if (buffer) {
  //   addSignal(pid,data);
  // }
}

template <typename SignalT>
void SignalHolder<SignalT>::addListener(PipeType const& pid, ListenerType&& cb) {
  auto iter = listeners_.find(pid);
  if (iter == listeners_.end()) {
    listeners_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(pid),
      std::forward_as_tuple(ListenerListType{})
    );
    iter = listeners_.find(pid);
    assert(iter != listeners_.end() && "Must exist now");
  }

  // // Deliver any pending signals
  // auto pending_iter = pending_holder_.find(pid);
  // if (pending_iter !=  pending_holder_.end()) {
  //   for (auto&& elm : pending_iter->second) {
  //     cb->trigger(elm,pid);
  //   }
  // }

  // Finally insert into listeners
  iter->second.emplace_back(std::move(cb));
}

template <typename SignalT>
void SignalHolder<SignalT>::applySignal(
  ListenerPtrType listener, DataPtrType data, PipeType const& pid
) {
  listener->trigger(data,pid);
}

template <typename SignalT>
bool SignalHolder<SignalT>::finished(ListenerPtrType listener) const {
  return listener->finished();
}

template <typename SignalT>
bool SignalHolder<SignalT>::exists(PipeType const& pipe) const {
  return listeners_.find(pipe) != listeners_.end();
}

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_IMPL_H*/
