/*
//@HEADER
// *****************************************************************************
//
//                           message_priority.impl.h
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

#if !defined INCLUDED_VT_MESSAGING_MESSAGE_MESSAGE_PRIORITY_IMPL_H
#define INCLUDED_VT_MESSAGING_MESSAGE_MESSAGE_PRIORITY_IMPL_H

#include "vt/config.h"

namespace vt { namespace messaging {

template <typename MsgT>
void msgSetPriorityLevel(MsgT ptr, PriorityLevelType level) {
# if backend_check_enabled(priorities)
  envelopeSetPriorityLevel(ptr->env, level);
# endif
}

template <typename MsgT>
void msgSetPriorityAllLevels(MsgT ptr, PriorityType priority) {
# if backend_check_enabled(priorities)
  envelopeSetPriority(ptr->env, priority);
# endif
}

template <typename MsgT, typename MsgU>
bool msgIncPriorityLevel(MsgT old_msg, MsgU new_msg) {
# if backend_check_enabled(priorities)
  auto const level = envelopeGetPriorityLevel(old_msg->env);
  if (level + 1 < sched::priority_num_levels) {
    envelopeSetPriorityLevel(new_msg->env, level + 1);
    return true;
  } else {
    return false;
  }
# endif
}

template <typename MsgU>
void msgSetPriority(MsgU new_msg, PriorityType priority, bool increment_level) {
# if backend_check_enabled(priorities)
  PriorityLevelType const level = increment_level ? 1 : 0;
  PriorityType old_priority = vt::min_priority;
  return msgSetPriorityImpl<MsgU>(new_msg, priority, old_priority, level);
# endif
}

template <typename MsgU>
void msgSetPriorityImpl(
  MsgU new_msg, PriorityType new_priority, PriorityType old_priority,
  PriorityLevelType level
) {
# if backend_check_enabled(priorities)

  bool const is_breadth_first = new_priority == sched::Priority::BreadthFirst;
  debug_print(
    gen, node,
    "msgSetPriorityImpl: new_priority={:x}, old_priority={:x}, level={}, "
    "is_breadth_first={}\n",
    new_priority, old_priority, level, is_breadth_first
  );
  for (PriorityType l = level + 1; l < sched::priority_num_levels; l++) {
    if (is_breadth_first) {
      sched::PriorityManip::setPriority(old_priority, l, sched::priority_all_set);
    } else {
      sched::PriorityManip::setPriority(old_priority, l, 0);
    }
  }
  sched::PriorityManip::setPriority(old_priority, level, new_priority);
  envelopeSetPriorityLevel(new_msg->env, level);
  envelopeSetPriority(new_msg->env, old_priority);
# endif
}

template <typename MsgT, typename MsgU>
void msgSetPriorityFrom(
  MsgT old_msg, MsgU new_msg, PriorityType priority, bool increment_level
) {
# if backend_check_enabled(priorities)
  vtAssert(old_msg != nullptr, "Must have a valid message");
  vtAssert(new_msg != nullptr, "Must have a valid message");
  PriorityLevelType level = envelopeGetPriorityLevel(old_msg->env);
  if (increment_level and level + 1 < sched::priority_num_levels) {
    level += 1;
  }
  PriorityType old_priority = envelopeGetPriority(old_msg->env);
  return msgSetPriorityImpl<MsgU>(new_msg, priority, old_priority, level);
# endif
}

template <typename MsgT>
void msgSystemSetPriority(MsgT ptr, PriorityType priority) {
# if backend_check_enabled(priorities)
  PriorityType prior = no_priority;
  envelopeSetPriorityLevel(ptr->env, 0);
  sched::PriorityManip::setPriority(prior, 0, priority);
  envelopeSetPriority(ptr->env, prior);
# endif
}

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_MESSAGE_MESSAGE_PRIORITY_IMPL_H*/
