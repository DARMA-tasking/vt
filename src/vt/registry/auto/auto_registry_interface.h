/*
//@HEADER
// *****************************************************************************
//
//                          auto_registry_interface.h
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

#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_INTERFACE_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_INTERFACE_H

#include "vt/registry/auto/auto_registry_common.h"
#include "vt/config.h"
#include "vt/registry/registry.h"

namespace vt { namespace auto_registry {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
HandlerType makeAutoHandler(MessageT* const msg);

template <typename T, T value>
HandlerType makeAutoHandlerParam();

template <typename T, bool is_msg, typename... Args>
HandlerType makeAutoHandlerFunctor();

AutoActiveType getAutoHandler(HandlerType const& handler);

AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& handler);

#if backend_check_enabled(trace_enabled)
  trace::TraceEntryIDType theTraceID(
    HandlerType const& handler, RegistryTypeEnum reg_type
  );
#endif

}} // end namespace vt::auto_registry

#include "vt/registry/auto/auto_registry.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"
#include "vt/registry/auto/index/auto_registry_index.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_INTERFACE_H*/
