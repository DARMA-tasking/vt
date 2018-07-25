
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
  using DataType             = typename SignalT::DataType;
  using SignalListType       = std::vector<SignalT>;
  using ListenerBaseType     = callback::CallbackBase<SignalT>;
  using ListenerType         = std::unique_ptr<ListenerBaseType>;
  using ListenerListType     = std::vector<ListenerType>;
  using ListenerListIterType = typename ListenerListType::iterator;
  using ListenerPtrType      = ListenerBaseType*;

  void addSignal(PipeType const& pid, DataType in_data);
  void removeListener(PipeType const& pid, ListenerPtrType listener);
  ListenerListIterType removeListener(ListenerListIterType iter);
  void clearAllListeners(PipeType const& pid);
  void clearAllSignals(PipeType const& pid);
  void clearPipe(PipeType const& pid);
  void deliverAll(PipeType const& pid, DataType data);
  void addListener(PipeType const& pid, ListenerType&& cb);
  void applySignal(ListenerPtrType listener, DataType data);
  bool finished(ListenerPtrType listener) const;

private:
  std::unordered_map<PipeType,SignalListType> pending_holder_;
  std::unordered_map<PipeType,ListenerListType> listeners_;
};

}}} /* end namespace vt::pipe::signal */

#include "pipe/signal/signal_holder.impl.h"

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H*/
