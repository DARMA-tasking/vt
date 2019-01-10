/*
//@HEADER
// ************************************************************************
//
//                          seq_context.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
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

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_node.h"
#include "vt/sequence/seq_context.h"
#include "vt/sequence/seq_ult_context.h"

#include <cassert>

namespace vt { namespace seq {

SeqContext::SeqContext(
  SeqType const& in_seq_id, SeqNodePtrType in_node, bool is_suspendable
) : suspendable_(is_suspendable), node_(in_node), seq_id(in_seq_id)
{
  debug_print(
    sequence, node,
    "SeqContext: construct: node={}, id={}, suspendable={}\n",
    PRINT_SEQ_NODE_PTR(node_), seq_id, print_bool(suspendable_)
  );

  if (suspendable_) {
    seq_ult = std::make_unique<SeqContextULTType>();
  }
}
SeqContext::SeqContext(SeqType const& in_seq_id) : seq_id(in_seq_id) { }

void SeqContext::suspend() {
  vtAssert(seq_ult != nullptr, "Seq ULT must be live");
  seq_ult->suspend();
}

void SeqContext::resume() {
  vtAssert(seq_ult != nullptr, "Seq ULT must be live");
  seq_ult->resume();
}

SeqType SeqContext::getSeq() const {
  return seq_id;
}

SeqNodePtrType SeqContext::getNode() const {
  return node_;
}

void SeqContext::setNode(SeqNodePtrType node) {
  debug_print(
    sequence, node,
    "SeqContext: setNode: node={}, id={}\n", PRINT_SEQ_NODE_PTR(node), seq_id
  );

  node_ = node;
}

bool SeqContext::isSuspendable() const {
  return suspendable_;
}

void SeqContext::setSuspendable(bool const is_suspendable) {
  suspendable_ = is_suspendable;
}

}} //end namespace vt::seq
