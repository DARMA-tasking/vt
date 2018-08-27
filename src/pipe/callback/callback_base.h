
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_BASE_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_BASE_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "context/context.h"

namespace vt { namespace pipe { namespace callback {

static struct CallbackPersistTagType   {} CallbackPersistTag   {};
static struct CallbackSingleUseTagType {} CallbackSingleUseTag {};
static struct CallbackMultiUseTagType  {} CallbackMultiUseTag  {};
static struct CallbackExplicitTagType  {} CallbackExplicitTag  {};

template <typename SignalT>
struct CallbackBase {
  using SignalType = SignalT;
  using SignalDataType = typename SignalT::DataType;

  CallbackBase(CallbackBase const&) = default;
  CallbackBase(CallbackBase&&) = default;
  CallbackBase& operator=(CallbackBase const&) = default;

  CallbackBase() :
    CallbackBase(CallbackSingleUseTag)
  { }
  explicit CallbackBase(CallbackPersistTagType)
    : reference_counted_(false)
  { }
  explicit CallbackBase(CallbackSingleUseTagType)
    : reference_counted_(true),
      refs_(1)
  { }
  CallbackBase(CallbackMultiUseTagType, RefType const& in_num_refs)
    : reference_counted_(true),
      refs_(in_num_refs)
  { }
  CallbackBase(
    CallbackExplicitTagType, bool const& persist, RefType const& in_refs = -1
  ) : reference_counted_(!persist), refs_(in_refs)
  { }

  virtual ~CallbackBase() = default;

public:
  bool expired() const { return reference_counted_ && refs_ < 1; }

protected:
  virtual void trigger_(SignalDataType* data) = 0;

  virtual void trigger_(SignalDataType* data, PipeType const& pipe_id) {
    return trigger_(data);
  }

public:
  void trigger(SignalDataType* data, PipeType const& pipe_id) {
    if (reference_counted_) {
      refs_--;
      triggered_++;
    }
    debug_print(
      pipe, node,
      "CallbackBase: (before) invoke trigger_: pipe={:x}\n",
      pipe_id
    );
    trigger_(data,pipe_id);
    debug_print(
      pipe, node,
      "CallbackBase: (after) invoke trigger_: pipe={:x}\n",
      pipe_id
    );
  }

  bool finished() const {
    if (reference_counted_) {
      vtAssert(refs_ != -1, "refs_ must be a valid positive integer");
      return refs_ == 0;
    } else {
      return false;
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | reference_counted_ | triggered_ | refs_;
  }

private:
  bool reference_counted_ = true;
  RefType triggered_      = 0;
  RefType refs_           = 1;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_BASE_H*/
