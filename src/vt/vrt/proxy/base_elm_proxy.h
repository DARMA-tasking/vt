/*
//@HEADER
// ************************************************************************
//
//                          base_elm_proxy.h
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

#if !defined INCLUDED_VRT_PROXY_BASE_ELM_PROXY_H
#define INCLUDED_VRT_PROXY_BASE_ELM_PROXY_H

#include "vt/config.h"

#include <cstdlib>
#include <cstdint>
#include <utility>

namespace vt { namespace vrt { namespace collection {

static struct virtual_proxy_elm_empty { } virtual_proxy_elm_empty_tag { };

template <typename ColT, typename IndexT>
struct BaseElmProxy {
  using IndexType = IndexT;
  using CollectionType = ColT;

  explicit BaseElmProxy(IndexT const& in_idx)
    : idx_(in_idx)
  { }
  explicit BaseElmProxy(virtual_proxy_elm_empty) { }

  BaseElmProxy() = default;
  BaseElmProxy(BaseElmProxy const&) = default;
  BaseElmProxy(BaseElmProxy&&) = default;
  BaseElmProxy& operator=(BaseElmProxy const&) = default;

  bool operator==(BaseElmProxy const& other) const {
    return other.idx_ == idx_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | idx_;
  }

  IndexT const& getIndex() const { return idx_; }

protected:
  IndexT idx_;
};

}}} /* end namespace vt::vrt::collection */

template <typename ColT, typename IndexT>
using ElmType = ::vt::vrt::collection::BaseElmProxy<ColT,IndexT>;

namespace std {
  template <typename ColT, typename IndexT>
  struct hash<ElmType<ColT,IndexT>> {
    size_t operator()(ElmType<ColT,IndexT> const& in) const {
      return std::hash<typename ElmType<ColT,IndexT>::IndexType>()(
        in.getIndex()
      );
    }
  };
}

#endif /*INCLUDED_VRT_PROXY_BASE_ELM_PROXY_H*/
