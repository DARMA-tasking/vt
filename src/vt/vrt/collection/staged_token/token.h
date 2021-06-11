/*
//@HEADER
// *****************************************************************************
//
//                                   token.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_H
#define INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT = typename ColT::IndexType>
struct InsertTokenRval {
  InsertTokenRval(VirtualProxyType const& in_proxy, IndexT in_idx)
    : proxy_(in_proxy),
      idx_(in_idx)
  { }
  InsertTokenRval(InsertTokenRval const&) = delete;
  InsertTokenRval(InsertTokenRval&&) = default;

  template <typename... Args>
  void insert(Args&&... args);

  void insertPtr(std::unique_ptr<ColT> ptr);

  friend CollectionManager;

private:
  VirtualProxyType proxy_ = no_vrt_proxy;
  IndexT idx_ = {};
};

template <typename ColT, typename IndexT = typename ColT::IndexType>
struct InsertToken {
private:
  InsertToken() = default;
  explicit InsertToken(VirtualProxyType const& in_proxy)
    : proxy_(in_proxy)
  { }

public:
  virtual ~InsertToken() = default;

public:
  template <
    typename Tp,
    typename   = typename std::enable_if<
      std::is_convertible<
        typename IndexT::BuildIndexType, typename std::decay<Tp>::type
      >::value, Tp
    >::type
  >
  InsertTokenRval<ColT> operator[](Tp&& tp) const {
    return InsertTokenRval<ColT>{proxy_,IndexT{std::forward<Tp>(tp)}};
  }

  template <
    typename Tp, typename... Tn,
    typename   = typename std::enable_if<
      std::is_convertible<
        typename IndexT::BuildIndexType, typename std::decay<Tp>::type
      >::value, Tp
    >::type
  >
  InsertTokenRval<ColT> operator()(Tp&& tp, Tn&&... tn) const {
    return InsertTokenRval<ColT>{
      proxy_,IndexT{std::forward<Tp>(tp),std::forward<Tn>(tn)...}
    };
  }

  template <
    typename IndexU,
    typename   = typename std::enable_if<
      std::is_same<IndexT, typename std::decay<IndexU>::type>::value, IndexU
    >::type
  >
  InsertTokenRval<ColT> operator[](IndexU const& idx) const {
    return InsertTokenRval<ColT>{proxy_,idx};
  }

  template <
    typename IndexU,
    typename   = typename std::enable_if<
      std::is_same<IndexT, typename std::decay<IndexU>::type>::value, IndexU
    >::type
  >
  InsertTokenRval<ColT> operator()(IndexU const& idx) const {
    return InsertTokenRval<ColT>{proxy_,idx};
  }

  friend CollectionManager;

public:
  InsertToken(InsertToken const&) = delete;
  InsertToken(InsertToken&&) = default;

  VirtualProxyType getProxy() const { return proxy_; }

private:
  VirtualProxyType proxy_ = no_vrt_proxy;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_H*/
