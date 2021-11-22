/*
//@HEADER
// *****************************************************************************
//
//                               auto_registry.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_REGISTRY_AUTO_AUTO_REGISTRY_H
#define INCLUDED_VT_REGISTRY_AUTO_AUTO_REGISTRY_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"

#include "vt/trace/trace.h"
#include "vt/utils/demangle/demangle.h"

#include "vt/activefn/activefn.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/objgroup/active_func/active_func.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

AutoActiveType const& getAutoHandler(HandlerType const handler);

AutoActiveObjGroupType getAutoHandlerObjGroup(HandlerType han);
AutoHandlerType getAutoHandlerObjTypeIdx(HandlerType han);

template <typename ObjT, typename MsgT, objgroup::ActiveObjType<MsgT, ObjT> f>
HandlerType makeAutoHandlerObjGroup(HandlerControlType control);

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
HandlerType makeAutoHandler(MsgT* const msg);

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
HandlerType makeAutoHandler();

template <typename T, T value>
HandlerType makeAutoHandlerParam();

template <typename ObjT, typename MsgT, objgroup::ActiveObjType<MsgT, ObjT> f>
void setHandlerTraceNameObjGroup(
  HandlerControlType ctrl, std::string const& name, std::string const& parent = ""
);

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void setHandlerTraceName(std::string const& name, std::string const& parent = "");

template <typename T, T value>
void setHandlerTraceName(std::string const& name, std::string const& parent = "");

}} // end namespace vt::auto_registry

#include "vt/registry/auto/auto_registry_impl.h"

#endif /*INCLUDED_VT_REGISTRY_AUTO_AUTO_REGISTRY_H*/
