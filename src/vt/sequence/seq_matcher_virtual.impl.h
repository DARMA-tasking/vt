/*
//@HEADER
// *****************************************************************************
//
//                          seq_matcher_virtual.impl.h
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

#if !defined INCLUDED_VT_SEQUENCE_SEQ_MATCHER_VIRTUAL_IMPL_H
#define INCLUDED_VT_SEQUENCE_SEQ_MATCHER_VIRTUAL_IMPL_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_action_virtual.h"
#include "vt/sequence/seq_state_virtual.h"
#include "vt/vrt/context/context_vrtheaders.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

using namespace vrt;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasFirstElem(T& lst) {
  return lst.size() > 0;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ auto SeqMatcherVirtual<VcT, MsgT, f>::getFirstElem(T& lst) {
  vtAssert(lst.size() > 0, "Must have element");
  auto elm = lst.front();
  lst.pop_front();
  return elm;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingAnyNoTag(
  SeqStateContType<T>& lst
) {
  return hasFirstElem(lst);
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ auto SeqMatcherVirtual<VcT, MsgT, f>::getMatchingAnyNoTag(
  SeqStateContType<T>& lst
) {
  return getFirstElem(lst);
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingAnyTagged(
  SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
) {
  auto iter = tagged_lst.find(tag);
  return iter != tagged_lst.end() ? hasFirstElem(iter->second) : false;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ auto SeqMatcherVirtual<VcT, MsgT, f>::getMatchingAnyTagged(
  SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
) {
  vtAssert(hasMatchingAnyTagged(tagged_lst, tag), "Must have matching elem");

  auto iter = tagged_lst.find(tag);
  auto elm = getFirstElem(iter->second);
  if (iter->second.size() == 0) {
    tagged_lst.erase(iter);
  }
  return elm;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingMsg(TagType const& tag) {
  if (tag == no_tag) {
#pragma sst global seq_msg
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg;
    return hasMatchingAnyNoTag(lst);
  } else {
#pragma sst global seq_msg_tagged
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg_tagged;
    return hasMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ MsgSharedPtr<MsgT>
SeqMatcherVirtual<VcT, MsgT, f>::getMatchingMsg(TagType const& tag) {
  if (tag == no_tag) {
#pragma sst global seq_msg
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg;
    return getMatchingAnyNoTag(lst);
  } else {
#pragma sst global seq_msg_tagged
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg_tagged;
    return getMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingAction(TagType const& tag) {
  if (tag == no_tag) {
#pragma sst global seq_action
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action;
    return hasMatchingAnyNoTag(lst);
  } else {
#pragma sst global seq_action_tagged
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action_tagged;
    return hasMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ typename SeqMatcherVirtual<VcT, MsgT, f>::SeqActionType
SeqMatcherVirtual<VcT, MsgT, f>::getMatchingAction(TagType const& tag) {
  vtAssert(hasMatchingAction(tag), "Must have matching action");

  if (tag == no_tag) {
#pragma sst global seq_action
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action;
    return getMatchingAnyNoTag(lst);
  } else {
#pragma sst global seq_action_tagged
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action_tagged;
    return getMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ void SeqMatcherVirtual<VcT, MsgT, f>::bufferUnmatchedMessage(
  MsgSharedPtr<MsgT> msg, TagType const& tag
) {
  if (tag == no_tag) {
#pragma sst global seq_msg
    SeqStateVirtualType<VcT, MsgT, f>::seq_msg.push_back(msg);
  } else {
#pragma sst global seq_msg_tagged
    SeqStateVirtualType<VcT, MsgT, f>::seq_msg_tagged[tag].push_back(msg);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename FnT>
/*static*/ void SeqMatcherVirtual<VcT, MsgT, f>::bufferUnmatchedAction(
  FnT action, SeqType const& seq_id, TagType const& tag
) {
  vt_debug_print(
    verbose, sequence,
    "SeqMatcher: buffering action: seq={}, tag={}\n", seq_id, tag
  );

  if (tag == no_tag) {
#pragma sst global seq_action
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action;
    lst.emplace_back(SeqActionType{seq_id,action});
  } else {
#pragma sst global seq_action_tagged
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action_tagged;
    tagged_lst[tag].emplace_back(SeqActionType{seq_id,action});
  }
}

}} //end namespace vt::seq

#endif /* INCLUDED_VT_SEQUENCE_SEQ_MATCHER_VIRTUAL_IMPL_H*/
