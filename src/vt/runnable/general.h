/*
//@HEADER
// *****************************************************************************
//
//                                  general.h
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

#if !defined INCLUDED_RUNNABLE_GENERAL_H
#define INCLUDED_RUNNABLE_GENERAL_H

#include "vt/config.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"

namespace vt {

namespace objgroup {

void scheduleMsg(
  MsgSharedPtr<ShortMessage> msg, HandlerType han, EpochType epoch
);

} /* end namespace objgroup */

namespace runnable {

template <typename MsgT>
struct Runnable {
  template <typename... Args>
  using FnParamType = void(*)(Args...);

  // Dispatch for normal active message handlers (functors, fn pointer, etc.)
  static void run(
    HandlerType handler, ActiveFnPtrType func, MsgT* msg, NodeType from_node,
    TagType in_tag = no_tag
  );

  friend void objgroup::scheduleMsg(
    MsgSharedPtr<ShortMessage> msg, HandlerType han, EpochType epoch
  );

private:
  // Dispatch for object groups: handler with node-local object ptr
  static void runObj(HandlerType handler, MsgT* msg, NodeType from_node);
};

struct RunnableVoid {
  template <typename... Args>
  using FnParamType = void(*)(Args...);

  static inline void run(HandlerType handler, NodeType from_node);
};

}} /* end namespace vt::runnable */

#include "vt/runnable/general.impl.h"

#endif /*INCLUDED_RUNNABLE_GENERAL_H*/
