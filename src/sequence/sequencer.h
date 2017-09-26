
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
#include <vector>
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
  using SeqFuncContainerType = std::vector<FuncType>;

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

  // compile-time list of parallel functions
  template <typename... FnT>
  void parallel(FnT&&... fns);
  template <typename... FnT>
  void parallel(SeqType const& seq_id, FnT&&... fns);

  // runtime dynamic list of parallel functions
  void parallel_lst(SeqFuncContainerType const& fn_list);
  void parallel_lst(SeqType const& seq_id, SeqFuncContainerType const& fn_list);

  void dispatch_parallel(
    bool const& has_context, SeqType const& seq_id, SeqNodePtrType par_node
  );

  void enqueueSeqList(SeqType const& seq_id);
  SeqType getCurrentSeq() const;
  bool scheduler();
  bool isLocalTerm();

  SeqNodePtrType getNode(SeqType const& id) const;
  SeqType getSeqID() const;

  // the general wait function
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait_on_trigger(TagType const& tag, SeqActionType<MessageT> action);

  // Wait functions that do not have state (they can be easily migrated if they
  // are registered)
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait(SeqTriggerType<MessageT> trigger);
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait(TagType const& tag, SeqTriggerType<MessageT> trigger);

  // Closure-based wait functions that have state and cannot be migrated easily
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait_closure(TagType const& tag, SeqNonMigratableTriggerType<MessageT> trigger);
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait_closure(SeqNonMigratableTriggerType<MessageT> trigger);

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

