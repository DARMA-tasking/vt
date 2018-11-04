
#if !defined INCLUDED_PIPE_SIGNAL_SIGNAL_H
#define INCLUDED_PIPE_SIGNAL_SIGNAL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"

#include <cstdlib>

namespace vt { namespace pipe { namespace signal {

struct SignalBase {};

template <typename DataT>
struct Signal : SignalBase {
  using DataType    = DataT;
  using DataPtrType = DataType*;

  Signal() = default;
  Signal(Signal const&) = default;
  Signal(Signal&&) = default;
  Signal& operator=(Signal const&) = default;

  explicit Signal(DataPtrType in_ptr)
    : Signal(in_ptr,no_tag)
  { }
  Signal(DataPtrType in_ptr, TagType in_tag)
    : data_ptr_(in_ptr), signal_tag_(in_tag)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | signal_tag_;
  }

public:
  DataPtrType get() const { return data_ptr_; }
  TagType getTag() const { return signal_tag_; }

private:
  DataPtrType data_ptr_ = nullptr;
  TagType signal_tag_ = no_tag;
};

using SigVoidType = int8_t;
using SignalVoid = Signal<SigVoidType>;

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_H*/
