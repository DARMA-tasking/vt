/*
//@HEADER
// *****************************************************************************
//
//                                 sub_handle.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H
#define INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H

#include "vt/configs/types/types_type.h"
#include "vt/rdmahandle/handle.h"
#include "vt/rdmahandle/index_info.h"
#include "vt/rdmahandle/cache.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/topos/mapping/dense/dense.h"

#include <unordered_map>

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename IndexT>
struct SubHandle {
  using HandleType = Handle<T, vt::rdma::HandleEnum::StaticSize, vt::Index2D>;
  using ProxyType = vt::objgroup::proxy::Proxy<SubHandle<T, E, IndexT>>;

  SubHandle() = default;

private:
  struct SubInfo {
    SubInfo() = default;
    SubInfo(uint64_t in_size, uint64_t in_offset)
      : size_(in_size),
        offset_(in_offset)
    { }
    uint64_t size_ = 0;
    uint64_t offset_ = 0;
  };

public:
  template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
  void initialize(ProxyType in_proxy, bool in_is_static, IndexT in_range);

  void makeSubHandles();

  typename IndexT::DenseIndexType linearize(IndexT idx);

  NodeType getHomeNode(IndexT const& idx);

  IndexInfo fetchInfo(IndexT const& idx);

  IndexInfo resolveLocation(IndexT const& idx);

  void get(IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset);

  std::size_t getSize(IndexT const& idx, Lock l = Lock::Shared);

  RequestHolder rget(
    IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
  );

  RequestHolder rput(
    IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
  );

  void put(IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset);

  RequestHolder raccum(
    IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset,
    MPI_Op op
  );

  void accum(
    IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset,
    MPI_Op op
  );

  bool isUniform() const { return uniform_size_; }

  template <typename Callable>
  void access(IndexT idx, Lock l, Callable fn, uint64_t offset);

  Handle<T, E, IndexT> addLocalIndex(IndexT index, uint64_t size);

  bool ready() const { return ready_; }

  uint64_t totalLocalSize() const;

  template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
  static ProxyType construct(bool in_is_static, IndexT in_range);

private:
  template <typename U>
  void waitForHandleReady(Handle<U,E> const& h);

protected:
  ProxyType proxy_;
  bool is_static_ = false;
  vt::HandlerType map_han_ = 0;
  IndexT range_ = {};
  std::unordered_map<IndexT, SubInfo> sub_handles_;
  std::vector<IndexT> sub_layout_;
  std::vector<uint64_t> sub_prefix_;
  Handle<T, E> data_handle_;
  Handle<uint64_t, E> loc_handle_;
  Cache<IndexT> cache_;
  bool ready_ = false;
  bool mpi2_ = false;
  bool uniform_size_ = false;
};

}} /* end namespace vt::rdma */

#include "vt/rdmahandle/sub_handle.h"

#endif /*INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H*/
