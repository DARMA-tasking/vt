/*
//@HEADER
// *****************************************************************************
//
//                          context_vrt_make_closure.h
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

#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_MAKE_CLOSURE_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_MAKE_CLOSURE_H

#include "vt/config.h"
#include "vt/vrt/context/context_vrt_fwd.h"
#include "vt/vrt/context/context_vrt_attorney.h"
#include "vt/vrt/proxy/proxy_bits.h"
#include "vt/vrt/context/context_vrtinfo.h"

#include <tuple>
#include <utility>
#include <type_traits>
#include <memory>

namespace vt { namespace vrt {

template <typename VrtCtxT>
using VrtPtrType = std::unique_ptr<VrtCtxT>;

struct VirtualConstructor {
  template <typename VrtCtxT, typename Tuple, size_t... I>
  static VrtPtrType<VrtCtxT> construct(Tuple* tup, std::index_sequence<I...>);
};

template <typename VrtContextT, typename... Args>
struct VirtualMakeClosure {
  using TupleType = std::tuple<Args...>;

  TupleType tup;
  VirtualProxyType proxy = no_vrt_proxy;
  VirtualInfo* info = nullptr;

  VirtualMakeClosure(
    TupleType&& in_tup, VirtualProxyType const& in_proxy, VirtualInfo* in_info
  ) : tup(std::forward<TupleType>(in_tup)), proxy(in_proxy), info(in_info)
  { }
  VirtualMakeClosure(VirtualMakeClosure&&) = default;

  void make() {
    debug_print(
      vrt, node,
      "VirtualMakeClosure: calling make()\n"
    );

    static constexpr auto args_size = std::tuple_size<TupleType>::value;
    auto vc_ptr = VirtualConstructor::construct<VrtContextT>(
      &tup, std::make_index_sequence<args_size>{}
    );

    VirtualContextAttorney::setProxy(vc_ptr.get(), proxy);
    info->setVirtualContextPtr(std::move(vc_ptr));
  }
};

}} /* end namespace vt::vrt */

#include "vt/vrt/context/context_vrt_make_closure.impl.h"

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_MAKE_CLOSURE_H*/
