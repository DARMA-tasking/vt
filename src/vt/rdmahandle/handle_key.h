/*
//@HEADER
// *****************************************************************************
//
//                                 handle_key.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_KEY_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_KEY_H

#include "vt/config.h"

namespace vt { namespace rdma {

struct HandleKey {
  struct {
    union {
      ObjGroupProxyType obj_;
      VirtualProxyType  vrt_;
    } u_;
    bool is_obj_;
  } proxy_ ;
  RDMA_HandleType handle_ = vt::no_rdma_handle;

  struct ObjGroupTag   { };
  struct CollectionTag { };

  bool isObjGroup() const { return proxy_.is_obj_; }
  RDMA_HandleType handle() const { return handle_; }
  bool valid() const { return handle_ != vt::no_rdma_handle; }

  friend bool operator==(HandleKey const& a1, HandleKey const& a2) {
    return
      a1.handle_ == a2.handle_ and
      a1.proxy_.is_obj_ == a2.proxy_.is_obj_ and
      (a1.proxy_.is_obj_ ?
       a1.proxy_.u_.obj_ == a2.proxy_.u_.obj_ :
       a1.proxy_.u_.vrt_ == a2.proxy_.u_.vrt_);
  }

  HandleKey() = default;
  HandleKey(ObjGroupTag, ObjGroupProxyType in_proxy, RDMA_HandleType in_handle)
    : handle_(in_handle)
  {
    proxy_.is_obj_ = true;
    proxy_.u_.obj_ = in_proxy;
  }
  HandleKey(CollectionTag, VirtualProxyType in_proxy, RDMA_HandleType in_handle)
    : handle_(in_handle)
  {
    proxy_.is_obj_ = false;
    proxy_.u_.vrt_ = in_proxy;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | handle_;

    s.countBytes(proxy_);
  }
};

}} /* end namespace vt::rdma */

namespace std {

template <>
struct hash<vt::rdma::HandleKey> {
  size_t operator()(vt::rdma::HandleKey const& in) const {
    return std::hash<uint64_t>()(
      in.handle_ ^
      (in.proxy_.is_obj_ ? in.proxy_.u_.obj_ : in.proxy_.u_.vrt_) ^
      (in.proxy_.is_obj_ ? 0x10 : 0x00)
    );
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_KEY_H*/
