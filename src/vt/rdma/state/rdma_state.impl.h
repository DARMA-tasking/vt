/*
//@HEADER
// *****************************************************************************
//
//                              rdma_state.impl.h
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

#if !defined INCLUDED_VT_RDMA_STATE_RDMA_STATE_IMPL_H
#define INCLUDED_VT_RDMA_STATE_RDMA_STATE_IMPL_H

#include "vt/config.h"
#include "vt/rdma/state/rdma_state.h"
#include "vt/messaging/message.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"
#include "vt/registry/auto/rdma/auto_registry_rdma.h"

namespace vt { namespace rdma {

template <typename MsgT, typename FuncT, ActiveTypedRDMAGetFnType<MsgT>* f>
RDMA_HandlerType State::setRDMAGetFn(
  MsgT* msg, FuncT const& fn, bool const& any_tag, TagType const& tag
) {
  vt_debug_print(
    normal, rdma_state,
    "setRDMAGetFn: GET tag={}, handle={}, any_tag={}\n",
    tag, handle, print_bool(any_tag)
  );

  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Get);

  auto const reg_han = auto_registry::makeAutoHandlerRDMAGet<MsgT,f>();

  if (any_tag) {
    vtAssert(
      tag == no_tag, "If any tag, you must not have a tag set"
    );
  }

  this_get_handler = reg_han;
  this_rdma_get_handler = handler;
  user_state_get_msg_ = msg;

  auto const& gen_fn = reinterpret_cast<RDMA_GetFunctionType>(fn);

  if (tag == no_tag) {
    rdma_get_fn = gen_fn;
    get_any_tag = any_tag;
  } else {
    get_tag_holder[tag] = RDMA_TagGetHolderType{gen_fn,handler,reg_han};
  }

  return handler;
}

template <typename MsgT, typename FuncT, ActiveTypedRDMAPutFnType<MsgT>* f>
RDMA_HandlerType State::setRDMAPutFn(
  MsgT* msg, FuncT const& fn, bool const& any_tag, TagType const& tag
) {
  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Put);

  vt_debug_print(
    normal, rdma_state,
    "setRDMAPutFn: PUT tag={}, handle={}, any_tag={}\n",
    tag, handle, print_bool(any_tag)
  );

  auto const reg_han = auto_registry::makeAutoHandlerRDMAPut<MsgT,f>();

  if (any_tag) {
    vtAssert(
      tag == no_tag, "If any tag, you must not have a tag set"
    );
  }

  this_put_handler = reg_han;
  this_rdma_put_handler = handler;
  user_state_put_msg_ = msg;

  auto const& gen_fn = reinterpret_cast<RDMA_PutFunctionType>(fn);

  if (tag == no_tag) {
    rdma_put_fn = gen_fn;
    put_any_tag = any_tag;
  } else {
    put_tag_holder[tag] = RDMA_TagPutHolderType{gen_fn,handler,reg_han};
  }

  return handler;
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMA_STATE_RDMA_STATE_IMPL_H*/
