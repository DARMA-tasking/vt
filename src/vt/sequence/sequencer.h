/*
//@HEADER
// *****************************************************************************
//
//                                 sequencer.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_SEQUENCE_SEQUENCER_H
#define INCLUDED_VT_SEQUENCE_SEQUENCER_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.h"
#include "vt/termination/termination.h"

#include "vt/sequence/sequencer_manager.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_context.h"
#include "vt/sequence/seq_node.h"
#include "vt/sequence/seq_list.h"
#include "vt/sequence/seq_state.h"
#include "vt/sequence/seq_matcher.h"
#include "vt/sequence/seq_action.h"
#include "vt/sequence/seq_parallel.h"
#include "vt/runtime/component/component_pack.h"

#include <unordered_map>
#include <list>
#include <vector>
#include <memory>
#include <cassert>

namespace vt { namespace seq {

/**
 * \struct TaggedSequencer
 *
 * \brief An experimental VT component for sequencing operations on a node.
 *
 * \warning This component is experimental and is not ready for production use.
 */
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
  using SeqCtxFunctionType = std::function<void()>;

  template <typename MessageT>
  using SeqActionType = Action<MessageT>;

  template <typename MessageT>
  using SeqTriggerType = SeqTrigger<MessageT>;

  template <typename T>
  using SeqIDContainerType = std::unordered_map<SeqType, T>;

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  using SeqStateMatcherType = SeqMatcher<MessageT, f>;

  using SeqManagerType = SeqManager<SeqTag, SeqTrigger>;

  static std::unique_ptr<SeqManagerType> seq_manager;

  TaggedSequencer() = default;
  virtual ~TaggedSequencer() {}

  // Get the correct ID based on the type
  virtual SeqType getNextID();

  SeqType nextSeq();
  SeqType createSeq();

  static SeqFunType convertSeqFun(SeqType const& id, UserSeqFunType fn);

  void assertValidContext() const;
  bool hasContext() const;

  void sequenced(FuncType const& fn);
  void sequenced(SeqType const& seq_id, FuncIDType const& fn);
  void sequenced(SeqType const& seq_id, FuncType const& fn);

  void for_loop(
    ForIndex const& begin, ForIndex const& end, ForIndex const& stride,
    FuncIndexType fn
  );

  // void forall_loop(
  //   ForIndex const& begin, ForIndex const& end, ForIndex const& stride,
  //   FuncIndexType fn
  // );

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
  bool isLocalTerm();

  SeqNodePtrType getNode(SeqType const& id) const;
  SeqType getSeqID() const;

  // the general wait function
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void wait_on_trigger(TagType const& tag, SeqActionType<MessageT> action);

  // Wait functions that do not have state (they can be easily migrated if they
  // are registered)
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void wait(SeqTriggerType<MessageT> trigger);
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void wait(TagType const& tag, SeqTriggerType<MessageT> trigger);

  // Closure-based wait functions that have state and cannot be migrated easily
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void wait_closure(TagType const& tag, SeqNonMigratableTriggerType<MessageT> trigger);
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void wait_closure(SeqNonMigratableTriggerType<MessageT> trigger);

  // @todo: should be made thread-safe and thread-local
  bool lookupContextExecute(SeqType const& id, SeqCtxFunctionType c);

  void storeNodeContext(SeqType const& id, SeqNodePtrType node);

  bool executeInNodeContext(
    SeqType const& id, SeqNodePtrType node, SeqCtxFunctionType c,
    bool const suspendable = false
  );
  bool executeSuspendableContext(
    SeqType const& id, SeqNodePtrType node, SeqCtxFunctionType c
  );

public:
  void enqueue(ActionType const& action);

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void sequenceMsg(MessageT* msg);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | context_
      | node_lookup_
      | seq_lookup_
      | seq_manager;
  }

private:
  SeqListType& getSeqList(SeqType const& seq_id);

protected:
  SeqContext* context_ = nullptr;

private:
  SeqContextContainerType node_lookup_;

  SeqIDContainerType<SeqListType> seq_lookup_;
};

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn);

struct Sequencer
  : runtime::component::Component<Sequencer>,
    TaggedSequencer<SeqType, SeqMigratableTriggerType>
{
  std::string name() override { return "Sequencer"; }

  template <typename Serializer>
  void serialize(Serializer& s) {}
};

#define SEQUENCE_REGISTER_HANDLER(message, handler)                     \
  static void handler(message* m) {                                     \
    theSeq()->sequenceMsg<message, handler>(m);                           \
  }

}} //end namespace vt::seq

namespace vt {

extern seq::Sequencer* theSeq();

} //end namespace vt

#include "vt/sequence/sequencer.impl.h"

#endif /*INCLUDED_VT_SEQUENCE_SEQUENCER_H*/

