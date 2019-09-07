/*
//@HEADER
// *****************************************************************************
//
//                                 seq_list.cc
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
#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_list.h"

namespace vt { namespace seq {

/*explicit*/ SeqList::SeqList(SeqType const& seq_id_in)
  : seq_id_(seq_id_in), ready_(true),
    node_(new SeqNode(seq_node_parent_tag_t, seq_id_))
{ }

void SeqList::addAction(SeqFunType const& fn) {
  debug_print(
    sequence, node,
    "SeqList: addAction id={}, node_={}\n", seq_id_, PRINT_SEQ_NODE_PTR(node_)
  );

  node_->addSequencedChild(SeqNodeType::makeNode(seq_id_, node_, fn));
}

void SeqList::addNode(SeqNodePtrType node) {
  debug_print(
    sequence, node,
    "SeqList: addNode id={}, node_={}\n", seq_id_, PRINT_SEQ_NODE_PTR(node_)
  );

  node_->addSequencedChild(std::move(node));
}

void SeqList::expandNextNode() {
  ready_ = false;

  auto const& state = node_->expandNext();

  debug_print(
    sequence, node,
    "SeqList: expandNextNode id={}, node_={}, state={}\n",
    seq_id_, PRINT_SEQ_NODE_PTR(node_), PRINT_SEQ_NODE_STATE(state)
  );

  switch (state) {
  case SeqNodeStateEnumType::WaitingNextState:
  case SeqNodeStateEnumType::NoMoreExpansionsState:
    return;
  case SeqNodeStateEnumType::KeepExpandingState:
    expandNextNode();
    break;
  default:
    vtAssert(0, "This should never happen");
  }
}

void SeqList::makeReady() {
  ready_ = true;
}

bool SeqList::isReady() const {
  return ready_;
}

SeqType SeqList::getSeq() const {
  return seq_id_;
}

}} //end namespace vt::seq
