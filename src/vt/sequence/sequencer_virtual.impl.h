/*
//@HEADER
// *****************************************************************************
//
//                           sequencer_virtual.impl.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_IMPL_H
#define INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_IMPL_H

#include "vt/config.h"
#include "vt/sequence/sequencer.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/vrt/context/context_vrtmanager.h"

#include <cassert>
#include <cstdint>

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
/*virtual*/ typename TaggedSequencerVrt<SeqTag, SeqTrigger>::SeqType
TaggedSequencerVrt<SeqTag, SeqTrigger>::getNextID() {
#pragma sst global seq_manager
  return this->seq_manager->nextSeqID(true);
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencerVrt<SeqTag, SeqTrigger>::SeqType
TaggedSequencerVrt<SeqTag, SeqTrigger>::createVirtualSeq(
  VirtualProxyType const& proxy
) {
  vt_debug_print(
    verbose, sequence_vrt,
    "SequencerVirtual: createSeqVrtContextt\n"
  );

  auto vrt_context = theVirtualManager()->getVirtualByProxy(proxy);

  vt_debug_print(
    verbose, sequence_vrt,
    "SequencerVirtual: lookup virtual context: vrt_context={}\n",
    print_ptr(vrt_context)
  );

  vtAssert(
    vrt_context != nullptr, "Virtual context must exist locally"
  );

  SeqType const& next_seq = this->createSeq();

  vt_debug_print(
    verbose, sequence_vrt,
    "SequencerVirtual: createSeqVrtContextt: new seq_id={}\n", next_seq
  );

  auto seq_iter = seq_vrt_lookup_.find(next_seq);

  vtAssert(
    seq_iter == seq_vrt_lookup_.end(), "Must not exist"
  );

  seq_vrt_lookup_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_seq),
    std::forward_as_tuple(VirtualInfoType{proxy})
  );

  return next_seq;
}

template <typename SeqTag, template <typename> class SeqTrigger>
VirtualProxyType TaggedSequencerVrt<SeqTag, SeqTrigger>::getCurrentVirtualProxy() {
  this->assertValidContext();

  auto cur_seq_id = this->context_->getSeq();

  vt_debug_print(
    verbose, sequence_vrt,
    "SequencerVirtual: getCurrentVrtProxy: seq={}\n", cur_seq_id
  );

  auto seq_iter = seq_vrt_lookup_.find(cur_seq_id);

  vtAssert(
    seq_iter != seq_vrt_lookup_.end(), "Must exist"
  );

  return seq_iter->second.proxy;
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::sequenceVrtMsg(
  MsgT* msg, VcT* vrt
) {
  auto const& is_tag_type = envelopeIsTagType(msg->env);
  TagType const& msg_tag = is_tag_type ? envelopeGetTag(msg->env) : no_tag;

  vt_debug_print(
    normal, sequence,
    "sequenceVrtMsg: arrived: msg={}, tag={}\n", print_ptr(msg), msg_tag
  );

  bool const has_match =
    SeqStateMatcherType<VcT, MsgT, f>::hasMatchingAction(msg_tag);

  vt_debug_print(
    verbose, sequence,
    "sequenceVrtMsg: arriving: msg={}, has_match={}, tag={}\n",
    print_ptr(msg), print_bool(has_match), msg_tag
  );

  // reference the arrival message to keep it alive past normal lifetime, in
  // case it's buffered or put into the scheduler
  auto pmsg = promoteMsg(msg);

  if (has_match) {
    auto action = SeqStateMatcherType<VcT, MsgT, f>::getMatchingAction(msg_tag);
    auto handle_msg_action = [this,action,pmsg,vrt]{
      this->lookupContextExecute(
        action.seq_id, action.generateCallable(pmsg.get(), vrt)
      );
    };

    // skip the work queue and directly execute
    if (seq_skip_queue) {
      handle_msg_action();
    } else {
      theVirtualSeq()->enqueue(handle_msg_action);
    }
  } else {
    // nothing was found so the message must be buffered and wait an action
    // being posted

    // buffer the unmatched messaged until a trigger is posted for it that
    // matches
    SeqStateMatcherType<VcT, MsgT, f>::bufferUnmatchedMessage(pmsg, msg_tag);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::wait(
  TagType const& tag, SeqTriggerType<MsgT, VcT> trigger
) {
  this->assertValidContext();
  return wait_on_trigger<VcT, MsgT, f>(
    tag, SeqActionType<MsgT, VcT>{this->getSeqID(),trigger}
  );
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::wait(
  SeqTriggerType<MsgT, VcT> trigger
) {
  return wait<VcT, MsgT, f>(no_tag, trigger);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::wait_on_trigger(
  TagType const& tag, SeqActionType<MsgT, VcT> action
) {
  theTerm()->produce();

  this->assertValidContext();

  SeqType const seq_id = this->getSeqID();
  SeqNodePtrType node = this->getNode(seq_id);

  vtAssert(node != nullptr, "Must have node from context");

  bool const seq_ready = node->isReady();

  vt_debug_print(
    verbose, sequence_vrt,
    "SequencerVrt: wait: tag={}: context seq id={}, node={}, blocked={}, "
    "ready={}\n",
    tag, seq_id, PRINT_SEQ_NODE_PTR(node),
    print_bool(node->isBlockedNode()), print_bool(seq_ready)
  );

  auto deferred_wait_action = [tag,action,node,seq_id]() -> bool {
    bool const has_match = SeqStateMatcherType<VcT,MsgT,f>::hasMatchingMsg(tag);

    if (has_match) {
      auto msg = SeqStateMatcherType<VcT, MsgT, f>::getMatchingMsg(tag);

      auto const& cur_proxy = theVirtualSeq()->getCurrentVirtualProxy();
      auto vrt_context = theVirtualManager()->getVirtualByProxy(cur_proxy);
      action.runAction(static_cast<VcT*>(vrt_context), msg.get());
    }

    vt_debug_print(
      verbose, sequence_vrt,
      "SequencerVrt: {}: tag={}: node={}, has_match={}, "
      "is_blocked={}\n",
      has_match ? "wait ran *immediately*" : "wait registered", tag,
      PRINT_SEQ_NODE_PTR(node), print_bool(has_match),
      print_bool(node->isBlockedNode())
    );

    bool const should_block = not has_match;

    node->setBlockedOnNode(eSeqConstructType::WaitConstruct, should_block);

    if (has_match) {
      vt_debug_print(
        verbose, sequence_vrt,
        "SequencerVrt: activating next node: seq={}, node={}, blocked={}\n",
        seq_id, PRINT_SEQ_NODE_PTR(node), print_bool(node->isBlockedNode())
      );

      node->activate();
    } else {
      // buffer the action to wait for a matching message

      auto msg_recv_trigger = [node,seq_id,action,tag](MsgT* msg, VcT* vc){
        vt_debug_print(
          verbose, sequence_vrt,
          "SequencerVrt: msg_recv_trigger: seq={}, tag={}, node={}, blocked={}, "
          "msg={}\n",
          seq_id, tag, PRINT_SEQ_NODE_PTR(node),
          print_bool(node->isBlockedNode()), print_ptr(msg)
        );

        action.runAction(vc, msg);

        vtAssert(node != nullptr, "node must not be nullptr");

        node->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
        node->activate();
      };

      // buffer the action because there is not a matching message to trigger
      // the message
      SeqStateMatcherType<VcT, MsgT, f>::bufferUnmatchedAction(
        msg_recv_trigger, seq_id, tag
      );
    }

    return should_block;
  };

  if (seq_ready) {
    // run it here, right now
    bool const has_match = not deferred_wait_action();

    vt_debug_print(
      verbose, sequence_vrt,
      "SequencerVrt: executed wait: has_match={}: seq_id={}\n",
      print_bool(has_match), seq_id
    );
  } else {
    node->addSequencedClosure(deferred_wait_action);
  }
}

}} //end namespace vt::seq

#endif /*INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_IMPL_H*/
