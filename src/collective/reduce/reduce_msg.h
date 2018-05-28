
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_MSG_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_MSG_H

#include "config.h"
#include "collective/reduce/reduce.fwd.h"
#include "messaging/message.h"

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
  HandlerType combine_handler_ = uninitialized_handler;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_MSG_H*/
