
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_MSG_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.fwd.h"
#include "vt/messaging/message.h"

#include <cstdlib>

namespace vt { namespace collective { namespace reduce {

struct ReduceMsg;

struct ReduceLink {
  using MsgCountType = uint16_t;

  template <typename T>
  T* getNext() const { return static_cast<T*>(next_); }

  bool isRoot() const { return is_root_; }
  MsgCountType getCount() const { return count_; }

  friend struct Reduce;

private:
  bool is_root_ = false;
  ReduceMsg* next_ = nullptr;
  MsgCountType count_ = 0;
};

struct ReduceMsg : ::vt::Message, ReduceLink {
  NodeType reduce_root_ = uninitialized_destination;
  TagType reduce_tag_ = no_tag;
  EpochType reduce_epoch_ = no_epoch;
  VirtualProxyType reduce_proxy_ = no_vrt_proxy;
  HandlerType combine_handler_ = uninitialized_handler;

  template <typename SerializerT>
  void invokeSerialize(SerializerT& s) {
    s | reduce_root_ | reduce_tag_ | reduce_epoch_ | reduce_proxy_;
    s | combine_handler_;
  }
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_MSG_H*/
