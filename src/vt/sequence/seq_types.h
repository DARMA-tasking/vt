/*
//@HEADER
// *****************************************************************************
//
//                                 seq_types.h
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

#if !defined INCLUDED_SEQUENCE_SEQ_TYPES_H
#define INCLUDED_SEQUENCE_SEQ_TYPES_H

#include <list>
#include <memory>
#include <functional>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"

namespace vt { namespace seq {

enum class eSeqNodeType : int8_t {
  ParentNode = 1,
  LeafNode = 2,
  ParallelNode = 3,
  InvalidNode = -1
};

enum class eSeqNodeOrderType : int8_t {
  SequencedOrder = 3,
  ParallelOrder = 4,
  InvalidOrder = -2
};

enum class eSeqNodeState : int8_t {
  WaitingNextState = 1,
  KeepExpandingState = 2,
  NoMoreExpansionsState = 3,
  InvalidState = -1
};

#define PRINT_SEQ_NODE_TYPE(NODE)                                       \
  ((NODE) == eSeqNodeType::ParentNode     ? "ParentNode"  :             \
    ((NODE) == eSeqNodeType::LeafNode      ? "LeafNode"    :            \
     ((NODE) == eSeqNodeType::ParallelNode ? "ParallelNode"    :        \
      ((NODE) == eSeqNodeType::InvalidNode ? "InvalidNode" : "???")     \
     )                                                                  \
    )                                                                   \
  )

#define PRINT_SEQ_NODE_ORDER(NODE)                                      \
  ((NODE) == eSeqNodeOrderType::SequencedOrder ? "SequencedOrder" :     \
   ((NODE) == eSeqNodeOrderType::ParallelOrder ? "ParallelOrder" :      \
    ((NODE) == eSeqNodeOrderType::InvalidOrder ? "InvalidOrder" : "???") \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_STATE(NODE)                                      \
  ((NODE) == eSeqNodeState::WaitingNextState ? "WaitingNextState" :     \
   ((NODE) == eSeqNodeState::KeepExpandingState ? "KeepExpandingState" : \
    ((NODE) == eSeqNodeState::NoMoreExpansionsState ? "NoMoreExpansionsState" : \
     ((NODE) == eSeqNodeState::InvalidState ? "InvalidState" : "???")   \
    )                                                                   \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_PTR(NODE)                                        \
  ((NODE) ? print_ptr((NODE).get()) : print_ptr(nullptr))

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_TYPES_H*/
