/*
//@HEADER
// ************************************************************************
//
//                          rdma_state.impl.h
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

#if !defined INCLUDED_RDMA_RDMA_STATE_IMPL_H
#define INCLUDED_RDMA_RDMA_STATE_IMPL_H

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
  auto const& this_node = theContext()->getNode();

  debug_print(
    rdma_state, node,
    "setRDMAGetFn: GET tag={}, handle={}, any_tag={}\n",
    tag, handle, print_bool(any_tag)
  );

  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Get);

  auto const reg_han = auto_registry::makeAutoHandlerRDMAGet<MsgT,f>(nullptr);

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

  auto const& this_node = theContext()->getNode();

  debug_print(
    rdma_state, node,
    "setRDMAPutFn: PUT tag={}, handle={}, any_tag={}\n",
    tag, handle, print_bool(any_tag)
  );

  auto const reg_han = auto_registry::makeAutoHandlerRDMAPut<MsgT,f>(nullptr);

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

#endif /*INCLUDED_RDMA_RDMA_STATE_IMPL_H*/
