/*
//@HEADER
// *****************************************************************************
//
//                                  rdmaable.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_RDMAABLE_RDMAABLE_H
#define INCLUDED_VT_VRT_COLLECTION_RDMAABLE_RDMAABLE_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/rdmahandle/handle.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct RDMAable : BaseProxyT {
  RDMAable() = default;
  RDMAable(RDMAable const&) = default;
  RDMAable(RDMAable&&) = default;
  RDMAable(VirtualProxyType const in_proxy);
  RDMAable& operator=(RDMAable const&) = default;

  /**
   * \brief Make a new RDMA handle for this collection---a collective invocation
   * across all elements
   *
   * \param[in] count the local count of T for this handle
   * \param[in] is_uniform whether all handles have the same count
   *
   * \return the new RDMA handle
   */
  template <typename T>
  vt::rdma::Handle<T, vt::rdma::HandleEnum::StaticSize, IndexT>
  makeHandleRDMA(IndexT idx, std::size_t count, bool is_uniform) const;

  /**
   * \brief Destroy an RDMA handle created for this collection
   *
   * \param[in] handle the handle to destroy
   */
  template <typename T, vt::rdma::HandleEnum E, typename IndexU>
  void destroyHandleRDMA(vt::rdma::Handle<T,E,IndexU> handle) const;

};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_RDMAABLE_RDMAABLE_H*/
