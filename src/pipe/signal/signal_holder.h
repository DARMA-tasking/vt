
#if !defined INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H
#define INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base.h"

#include <cassert>
#include <unordered_map>
#include <vector>
#include <memory>
#include <list>
#include <cstdlib>

namespace vt { namespace pipe { namespace signal {

template <typename SignalT>
struct SignalHolder {
  using DataType             = typename SignalT::DataType;
  using DataPtrType          = DataType*;
  using SigCountType         = int32_t;
  using SignalListType       = std::vector<SignalT>;
  using ListenerBaseType     = callback::CallbackBase<SignalT>;
  using ListenerType         = std::unique_ptr<ListenerBaseType>;
  using ListenerListType     = std::list<ListenerType>;
  using ListenerListIterType = typename ListenerListType::iterator;
  using ListenerPtrType      = ListenerBaseType*;
  using ListenerMapType      = std::unordered_map<PipeType,ListenerListType>;
  using SignalMapType        = std::unordered_map<PipeType,SignalListType>;
  using CountMapType         = std::unordered_map<PipeType,SigCountType>;
  using ListenerMapIterType  = typename ListenerMapType::iterator;

  void addSignal(PipeType const& pid, DataPtrType in_data);
  void removeListener(PipeType const& pid, ListenerPtrType listener);
  ListenerListIterType removeListener(
    ListenerMapIterType map_iter, ListenerListIterType iter
  );
  void clearAllListeners(PipeType const& pid);
  void clearAllSignals(PipeType const& pid);
  void clearPipe(PipeType const& pid);
  void deliverAll(PipeType const& pid, DataPtrType data);
  void addListener(PipeType const& pid, ListenerType&& cb);
  void setCount(PipeType const& pid, SigCountType const& count);
  SigCountType getCount(PipeType const& pid);
  void applySignal(
    ListenerPtrType listener, DataPtrType data, PipeType const& pid
  );
  bool finished(ListenerPtrType listener) const;
  bool exists(PipeType const& pipe) const;

private:
  SignalMapType pending_holder_;
  CountMapType listener_count_;
  ListenerMapType listeners_;
};

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H*/
