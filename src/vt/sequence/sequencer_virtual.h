/*
//@HEADER
// *****************************************************************************
//
//                             sequencer_virtual.h
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

#if !defined INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_H
#define INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_H

#include "vt/config.h"
#include "vt/sequence/sequencer.h"
#include "vt/sequence/seq_virtual_info.h"
#include "vt/sequence/seq_matcher_virtual.h"
#include "vt/sequence/seq_action_virtual.h"
#include "vt/sequence/seq_state_virtual.h"
#include "vt/vrt/context/context_vrt.h"

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencerVrt : TaggedSequencer<SeqTag, SeqTrigger> {
  using SeqType = SeqTag;
  using Base = TaggedSequencer<SeqTag,SeqTrigger>;
  using VirtualInfoType = VirtualInfo;

  template <typename MessageT, typename VcT>
  using SeqTriggerType = SeqMigratableVrtTriggerType<MessageT, VcT>;

  template <typename MessageT, typename VcT>
  using SeqActionType = ActionVirtual<MessageT, VcT>;

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  using SeqStateMatcherType = SeqMatcherVirtual<VcT, MsgT, f>;

  SeqType createVirtualSeq(VirtualProxyType const& proxy);
  VirtualProxyType getCurrentVirtualProxy();

  virtual SeqType getNextID() override;

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void sequenceVrtMsg(MsgT* msg, VcT* vrt_context);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void wait(TagType const& tag, SeqTriggerType<MsgT, VcT> trigger);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void wait(SeqTriggerType<MsgT, VcT> trigger);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void wait_on_trigger(TagType const& tag, SeqActionType<MsgT, VcT> action);

private:
  typename Base::template SeqIDContainerType<VirtualInfoType> seq_vrt_lookup_;
};

#define SEQUENCE_REGISTER_VRT_HANDLER(vrtcontext, message, handler)     \
  static void handler(message* m, vrtcontext* vc) {                     \
    theVirtualSeq()->sequenceVrtMsg<vrtcontext, message, handler>(m, vc); \
  }

using SequencerVirtual = TaggedSequencerVrt<SeqType, SeqMigratableTriggerType>;

}} //end namespace vt::seq

namespace vt {

extern seq::SequencerVirtual* theVirtualSeq();

} //end namespace vt

#include "vt/sequence/sequencer_virtual.impl.h"

#endif /*INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_H*/
