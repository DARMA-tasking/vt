/*
//@HEADER
// *****************************************************************************
//
//                          auto_registry_collection.h
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

#if !defined INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_H
#define INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"

namespace vt { namespace auto_registry {

using namespace ::vt::vrt::collection;

AutoActiveCollectionType getAutoHandlerCollection(HandlerType const handler);

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
HandlerType makeAutoHandlerCollection();

AutoActiveCollectionMemType getAutoHandlerCollectionMem(
  HandlerType const handler
);

template <
  typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
HandlerType makeAutoHandlerCollectionMem();

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
void setHandlerTraceNameColl(std::string const& name, std::string const& parent = "");

template <typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
void setHandlerTraceNameCollMem(std::string const& name, std::string const& parent = "");

}} /* end namespace vt::auto_registry */

#include "vt/registry/auto/collection/auto_registry_collection.impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_H*/
