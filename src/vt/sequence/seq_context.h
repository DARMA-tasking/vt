/*
//@HEADER
// ************************************************************************
//
//                          seq_context.h
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

#if !defined INCLUDED_SEQUENCE_SEQ_CONTEXT_H
#define INCLUDED_SEQUENCE_SEQ_CONTEXT_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_node.h"
#include "vt/sequence/seq_ult_context.h"

#include <list>
#include <memory>

#include <context/context_wrapper.h>

namespace vt { namespace seq {

struct SeqContext {
  using SeqContextULTType = SeqULTContext;
  using SeqContextULTPtrType = std::unique_ptr<SeqULTContext>;

  SeqContext(
    SeqType const& in_seq_id, SeqNodePtrType in_node, bool is_suspendable = false
  );
  SeqContext(SeqType const& in_seq_id);
  SeqContext(SeqContext const&) = delete;

  SeqType getSeq() const;
  SeqNodePtrType getNode() const;
  void setNode(SeqNodePtrType node);
  bool isSuspendable() const;
  void setSuspendable(bool const is_suspendable);
  void suspend();
  void resume();

  SeqContextULTPtrType seq_ult = nullptr;

private:
  bool suspendable_ = false;

  SeqNodePtrType node_ = nullptr;

  SeqType seq_id = no_seq;
};

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_CONTEXT_H*/
