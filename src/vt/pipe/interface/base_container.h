
#if !defined INCLUDED_PIPE_INTERFACE_BASE_CONTAINER_H
#define INCLUDED_PIPE_INTERFACE_BASE_CONTAINER_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/id/pipe_id.h"

namespace vt { namespace pipe { namespace interface {

template <typename MsgT>
struct BaseContainer {

  explicit BaseContainer(PipeType const& in_pipe)
    : pipe_(in_pipe)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | pipe_;
  }

  PipeType getPipe() const { return pipe_; }

private:
  PipeType pipe_ = no_pipe;
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_BASE_CONTAINER_H*/
