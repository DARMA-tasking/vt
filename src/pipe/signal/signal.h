
#if !defined INCLUDED_PIPE_SIGNAL_SIGNAL_H
#define INCLUDED_PIPE_SIGNAL_SIGNAL_H

#include "config.h"
#include "pipe/pipe_common.h"

namespace vt { namespace pipe { namespace signal {

struct SignalBase {};

template <typename MsgT>
struct Signal : SignalBase {
  using DataType = MsgT;

  TagType getTag() const { return signal_tag_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | signal_tag_;
  }

private:
  TagType signal_tag_ = no_tag;
};

using SignalVoid = Signal<void>;

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_H*/
