/*
//@HEADER
// *****************************************************************************
//
//                            seq_matcher_virtual.h
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

#if !defined INCLUDED_VT_SEQUENCE_SEQ_MATCHER_VIRTUAL_H
#define INCLUDED_VT_SEQUENCE_SEQ_MATCHER_VIRTUAL_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_action.h"
#include "vt/sequence/seq_state_virtual.h"
#include "vt/sequence/seq_action_virtual.h"
#include "vt/vrt/context/context_vrtheaders.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

using namespace vrt;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
struct SeqMatcherVirtual {
  using SeqActionType = ActionVirtual<MsgT, VcT>;
  using MatchFuncType = ActiveVrtTypedFnType<MsgT, VcT>;
  using SeqMsgStateType = SeqMsgStateVirtual<VcT, MsgT, f>;

  template <typename T>
  using SeqStateContType = typename SeqMsgStateType::template ContainerType<T>;

  template <typename T>
  using SeqStateTaggedContType = typename SeqMsgStateType::template TagContainerType<T>;

  template <typename T>
  static bool hasFirstElem(T& lst);
  template <typename T>
  static auto getFirstElem(T& lst);

  template <typename T>
  static bool hasMatchingAnyNoTag(SeqStateContType<T>& lst);
  template <typename T>
  static auto getMatchingAnyNoTag(SeqStateContType<T>& lst);

  template <typename T>
  static bool hasMatchingAnyTagged(
    SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
  );
  template <typename T>
  static auto getMatchingAnyTagged(
    SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
  );

  static bool hasMatchingAction(TagType const& tag);
  static SeqActionType getMatchingAction(TagType const& tag);

  static bool hasMatchingMsg(TagType const& tag);
  static MsgSharedPtr<MsgT> getMatchingMsg(TagType const& tag);

  // Buffer messages and actions that do not match
  static void bufferUnmatchedMessage(MsgSharedPtr<MsgT> msg, TagType const& tag);
  template <typename FnT>
  static void bufferUnmatchedAction(
    FnT action, SeqType const& seq_id, TagType const& tag
  );
};

}} //end namespace vt::seq

#include "vt/sequence/seq_matcher_virtual.impl.h"

#endif /* INCLUDED_VT_SEQUENCE_SEQ_MATCHER_VIRTUAL_H*/
