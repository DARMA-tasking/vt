/*
//@HEADER
// *****************************************************************************
//
//                                   handle.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_H

#include "vt/config.h"
#include "vt/rdmahandle/handle_key.h"
#include "vt/rdmahandle/request_holder.h"

#include <functional>

namespace vt { namespace rdma {

enum struct HandleEnum {
  StaticSize = 1,
  ConcurrentList = 2
};

struct BaseHandle { };

struct Manager;

template <typename T, HandleEnum E>
struct Handle : BaseHandle {
  using DataT = T;
  using RequestType = RequestHolder;

  static constexpr HandleEnum handle_type = E;

  Handle() = default;
  Handle(Handle const&) = default;
  Handle(Handle&&) = default;
  Handle& operator=(Handle const&) = default;

  friend struct Manager;

private:
  Handle(HandleKey in_key, std::size_t in_size)
    : key_(in_key), size_(in_size)
  { }

public:
  bool isInit() const { return key_.valid(); }
  bool ready() const;

public:
  void readExclusive(std::function<void(T const*)> fn);
  void readShared(std::function<void(T const*)> fn);
  void modifyExclusive(std::function<void(T*)> fn);
  void modifyShared(std::function<void(T*)> fn);

public:
  void get(vt::NodeType, T* ptr, std::size_t len, int offset);
  void put(vt::NodeType, T* ptr, std::size_t len, int offset);
  RequestType rget(vt::NodeType, T* ptr, std::size_t len, int offset);
  RequestType rput(vt::NodeType, T* ptr, std::size_t len, int offset);

protected:
  HandleKey key_ = {};
  std::size_t size_  = 0;
};

}} /* end namespace vt::rdma */

namespace vt {

template <typename T>
using HandleRDMA = rdma::Handle<T, rdma::HandleEnum::StaticSize>;

template <typename T>
using HandleListRDMA = rdma::Handle<T, rdma::HandleEnum::ConcurrentList>;

} /* end namespace vt */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_H*/
