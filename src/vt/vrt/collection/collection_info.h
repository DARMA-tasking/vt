/*
//@HEADER
// ************************************************************************
//
//                          collection_info.h
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

#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/context/context_vrt_fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionInfo {
  CollectionInfo() = default;
  CollectionInfo(CollectionInfo const&) = default;
  CollectionInfo(
    IndexT const& in_range, bool const in_immediate,
    NodeType const& in_from_node, VirtualProxyType in_proxy
  ) : immediate_(in_immediate), proxy_(in_proxy),
      from_node_(in_from_node), range_(in_range)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | immediate_ | proxy_ | req_id_ | from_node_ | range_ | insert_epoch_;
  }

  VirtualProxyType getProxy() const { return proxy_; }
  IndexT getRange() const { return range_; }

  void setInsertEpoch(EpochType const& in_epoch) { insert_epoch_ = in_epoch; }
  EpochType getInsertEpoch() const { return insert_epoch_; }

  friend struct CollectionManager;

private:
  bool immediate_ = false;
  VirtualProxyType proxy_ = no_vrt_proxy;
  VirtualRequestIDType req_id_ = no_request_id;
  NodeType from_node_ = uninitialized_destination;
  IndexT range_;
  EpochType insert_epoch_ = no_epoch;
};


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H*/
