/*
//@HEADER
// *****************************************************************************
//
//                               runnable.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_RUNNABLE_RUNNABLE_IMPL_H
#define INCLUDED_VT_RUNNABLE_RUNNABLE_IMPL_H

#include "vt/runnable/runnable.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"

namespace vt { namespace runnable {

template <>
inline ctx::SetContext* RunnableNew::get<ctx::SetContext>() {
  return &contexts_.setcontext;
}

template <>
inline ctx::LBData* RunnableNew::get<ctx::LBData>() {
  if (contexts_.has_lb)
    return &contexts_.lb;
  else
    return nullptr;
}

#if vt_check_enabled(trace_enabled)
template <>
inline ctx::Trace* RunnableNew::get<ctx::Trace>() {
  if (contexts_.has_trace)
    return &contexts_.trace;
  else
    return nullptr;
}
#endif


template <typename... Args>
void RunnableNew::addContextSetContext(Args&&... args) {
  contexts_.setcontext = ctx::SetContext{std::forward<Args>(args)...};
}

template <typename... Args>
void RunnableNew::addContextTD(Args&&... args) {
  contexts_.td = ctx::TD{std::forward<Args>(args)...};
  contexts_.has_td = true;
}

template <typename... Args>
void RunnableNew::addContextCont(Args&&... args) {
  contexts_.cont = ctx::Continuation{std::forward<Args>(args)...};
  contexts_.has_cont = true;
}

template <typename... Args>
void RunnableNew::addContextCol(Args&&... args) {
  contexts_.col = ctx::Collection{std::forward<Args>(args)...};
  contexts_.has_col = true;
}

template <typename... Args>
void RunnableNew::addContextLB(Args&&... args) {
  contexts_.lb = ctx::LBData{std::forward<Args>(args)...};
  contexts_.has_lb = true;
}

#if vt_check_enabled(trace_enabled)
template <typename... Args>
void RunnableNew::addContextTrace(Args&&... args) {
  contexts_.trace = ctx::Trace{std::forward<Args>(args)...};
  contexts_.has_trace = true;
}
#endif

}} /* end namespace vt::runnable */

#endif /*INCLUDED_VT_RUNNABLE_RUNNABLE_IMPL_H*/
