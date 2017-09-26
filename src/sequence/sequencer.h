
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE__
#define __RUNTIME_TRANSPORT_SEQUENCE__

#include "config.h"
#include "message.h"
#include "active.h"
#include "termination.h"
#include "concurrent_deque.h"

#include "seq_common.h"
#include "seq_context.h"
#include "seq_node.h"
#include "seq_list.h"
#include "seq_state.h"
#include "seq_matcher.h"
#include "seq_action.h"
#include "seq_parallel.h"

#include <unordered_map>
#include <list>
#include <cassert>

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencer {
  using SeqType = SeqTag;
  using SeqListType = SeqList;
  using SeqContextType = SeqContext;
  using SeqParallelType = SeqParallel;
  using SeqFunType = SeqListType::SeqFunType;
  using SeqContextPtrType = SeqContextType*;
  using SeqContextContainerType = std::unordered_map<SeqType, SeqNodePtrType>;

  template <typename MessageT>
  using SeqActionType = Action<MessageT>;

  template <typename MessageT>
  using SeqTriggerType = SeqTrigger<MessageT>;

  template <typename T>
  using SeqIDContainerType = std::unordered_map<SeqType, T>;

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  using SeqStateMatcherType = SeqMatcher<MessageT, f>;

  TaggedSequencer() = default;

  SeqType nextSeq();

  static SeqFunType convertSeqFun(SeqType const& id, UserSeqFunType fn);

  void assertValidContext() const;
  bool hasContext() const;

  void sequenced(FuncType const& fn);
  void sequenced(SeqType const& seq_id, FuncIDType const& fn);
  void sequenced(SeqType const& seq_id, FuncType const& fn);

  void parallel(FuncType fn1, FuncType fn2);
  void parallel(SeqType const& seq_id, FuncType fn1, FuncType fn2);

  void enqueueSeqList(SeqType const& seq_id);
  SeqType getCurrentSeq() const;
  bool scheduler();
  bool isLocalTerm();

  SeqNodePtrType getNode(SeqType const& id) const;
  SeqType getSeqID() const;

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait(SeqTriggerType<MessageT> trigger);

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait(TagType const& tag, SeqTriggerType<MessageT> trigger);

  // @todo: should be made thread-safe and thread-local
  template <typename Callable>
  bool lookupContextExecute(SeqType const& id, Callable&& c);

  void storeNodeContext(SeqType const& id, SeqNodePtrType node);

  template <typename Fn>
  bool executeInNodeContext(SeqType const& id, SeqNodePtrType node, Fn&& c);

public:
  void enqueue(ActionType const& action);

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void sequenceMsg(MessageT* msg);

private:
  SeqListType& getSeqList(SeqType const& seq_id);

private:
  SeqContextContainerType node_lookup_;

  SeqContext* context_ = nullptr;

  SeqIDContainerType<SeqListType> seq_lookup_;

  util::container::ConcurrentDeque<ActionType> work_deque_;
};

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn);

using Sequencer = TaggedSequencer<SeqType, SeqMigratableTriggerType>;

#define SEQUENCE_REGISTER_HANDLER(message, handler)                     \
  static void handler(message* m) {                                     \
    theSeq->sequenceMsg<message, handler>(m);                           \
  }

}} //end namespace vt::seq

namespace vt {

extern std::unique_ptr<seq::Sequencer> theSeq;

} //end namespace vt

#include "sequencer.impl.h"

#endif /*__RUNTIME_TRANSPORT_SEQUENCE__*/

