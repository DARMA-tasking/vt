
#if !defined INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H
#define INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base.h"

#include <cassert>
#include <unordered_map>
#include <vector>
#include <memory>

namespace vt { namespace pipe { namespace signal {

template <typename SignalT>
struct SignalHolder {
  using DataType = typename SignalT::DataType;
  using SignalListType = std::vector<SignalT>;
  using ListenerBaseType = callback::CallbackBase<SignalT>;
  using ListenerType = std::unique_ptr<ListenerBaseType>;
  using ListenerListType = std::vector<ListenerType>;
  using ListenerListIterType = typename ListenerListType::iterator;
  using ListenerPtrType = ListenerBaseType*;

  void addSignal(PipeType const& pid, DataType in_data) {
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

  void removeListener(PipeType const& pid, ListenerPtrType listener) {
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

  void clearAllListeners(PipeType const& pid) {
    auto iter = listeners_.find(pid);
    if (iter != listeners_.end()) {
      iter->second.clear();
    }
  }

  void clearAllSignals(PipeType const& pid) {
    auto iter = pending_holder_.find(pid);
    if (iter != pending_holder_.end()) {
      iter->second.clear();
    }
  }

  void clearPipe(PipeType const& pid) {
    clearAllListeners(pid);
    clearAllSignals(pid);
  }

  ListenerListIterType removeListener(ListenerListIterType iter) {
    ListenerType elm = std::move(*iter);
    auto niter = iter->second.erase(iter);
    // explicitly set to nullptr to invoke destructor (instead of waiting for
    // out of scope)
    elm = nullptr;
    return niter;
  }

  void deliverAll(PipeType const& pid, DataType data) {
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

  void addListener(PipeType const& pid, ListenerType&& cb) {
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

  void applySignal(ListenerPtrType listener, DataType data) {
    listener->trigger(data);
  }

  bool finished(ListenerPtrType listener) const {
    return listener->finished();
  }

private:
  std::unordered_map<PipeType,SignalListType> pending_holder_;
  std::unordered_map<PipeType,ListenerListType> listeners_;
};

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H*/
