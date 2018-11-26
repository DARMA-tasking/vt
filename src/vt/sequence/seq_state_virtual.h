/*
//@HEADER
// ************************************************************************
//
//                          seq_state_virtual.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_SEQUENCE_SEQ_STATE_VIRTUAL_H
#define INCLUDED_SEQUENCE_SEQ_STATE_VIRTUAL_H

#include <list>
#include <unordered_map>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_action_virtual.h"

namespace vt { namespace seq {

using namespace vrt;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
struct SeqMsgStateVirtual {
  using ActionType = ActionVirtual<MsgT, VcT>;

  template <typename T>
  using TagContainerType = std::unordered_map<TagType, T>;

  template <typename T>
  using ContainerType = std::list<T>;

  using ActionContainerType = ContainerType<ActionType>;
  using TaggedActionContainerType = TagContainerType<ActionContainerType>;

  using MsgContainerType = ContainerType<MsgT*>;
  using TaggedMsgContainerType = TagContainerType<std::list<MsgT*>>;

  // waiting actions on matching message arrival
  static ActionContainerType seq_action;
  static TaggedActionContainerType seq_action_tagged;

  // waiting messages on matching action arrival
  static MsgContainerType seq_msg;
  static TaggedMsgContainerType seq_msg_tagged;
};

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
using SeqStateVirtualType = SeqMsgStateVirtual<VcT, MsgT, f>;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqMsgStateVirtual<VcT, MsgT, f>::ActionContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_action;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqMsgStateVirtual<VcT, MsgT, f>::TaggedActionContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_action_tagged;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqMsgStateVirtual<VcT, MsgT, f>::MsgContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_msg;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqMsgStateVirtual<VcT, MsgT, f>::TaggedMsgContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_msg_tagged;

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_STATE_VIRTUAL_H*/
