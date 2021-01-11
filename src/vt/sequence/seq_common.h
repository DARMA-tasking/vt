/*
//@HEADER
// *****************************************************************************
//
//                                 seq_common.h
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

#if !defined INCLUDED_SEQUENCE_SEQ_COMMON_H
#define INCLUDED_SEQUENCE_SEQ_COMMON_H

#include <cstdint>
#include <functional>
#include <vector>

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/context/context_vrt_funcs.h"

namespace vt { namespace seq {

using SeqType = int32_t;
using UserSeqFunType = std::function<void()>;
using FuncType = UserSeqFunType;
using SystemSeqFunType = std::function<bool()>;
using UserSeqFunWithIDType = std::function<void(SeqType const&)>;
using FuncIDType = UserSeqFunWithIDType;
using SeqFuncContainerType = std::vector<FuncType>;
using ForIndex = int32_t;
using UserSeqFunIndexType = std::function<void(ForIndex idx)>;
using FuncIndexType = UserSeqFunIndexType;

// Regular sequence triggers for active message handlers
template <typename MessageT>
using SeqNonMigratableTriggerType = std::function<void(MessageT*)>;
template <typename MessageT>
using SeqMigratableTriggerType = ActiveTypedFnType<MessageT>;

// Specialized virtual context sequence triggers for VC active message handlers
template <typename MessageT, typename VcT>
using SeqNonMigratableVrtTriggerType = std::function<void(MessageT*, VcT*)>;
template <typename MessageT, typename VcT>
using SeqMigratableVrtTriggerType = vrt::ActiveVrtTypedFnType<MessageT, VcT>;

using SeqContinuation = std::function<void()>;

enum class eSeqConstructType : int8_t {
  WaitConstruct = 1,
  ParallelConstruct = 2,
  InvalidConstruct = -1
};

#define PRINT_SEQ_CONSTRUCT_TYPE(NODE)                                  \
  ((NODE) == eSeqConstructType::WaitConstruct ?"WaitConstruct" :        \
   ((NODE) == eSeqConstructType::ParallelConstruct ? "ParallelConstruct" : \
    ((NODE) == eSeqConstructType::InvalidConstruct ? "InvalidConstruct" : "???") \
   )                                                                    \
  )

using SeqCallableType = std::function<bool()>;

static constexpr SeqType const initial_seq = 0;
static constexpr SeqType const no_seq = -1;

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
);
bool contextualExecutionVirtual(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
);
void enqueueAction(SeqType const& id, ActionType const& action);

static constexpr bool const seq_skip_queue = false;

}} //end namespace vt::seq

namespace vt {

using SeqType = seq::SeqType;
using UserSeqFunType = seq::UserSeqFunType;
using UserSeqFunWithIDType = seq::UserSeqFunWithIDType;

} // end namespace vt

#endif /* INCLUDED_SEQUENCE_SEQ_COMMON_H*/
