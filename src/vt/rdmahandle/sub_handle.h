/*
//@HEADER
// *****************************************************************************
//
//                                 sub_handle.h
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

namespace impl {

struct ReduceLBMsg;

} /* end namespace impl */

template <typename T, HandleEnum E, typename IndexT>
struct SubHandle {
  using HandleType = Handle<T, vt::rdma::HandleEnum::StaticSize, IndexT>;
  using ProxyType = vt::objgroup::proxy::Proxy<SubHandle<T, E, IndexT>>;

  SubHandle() = default;

private:
  struct SubInfo {
    SubInfo() = default;
    SubInfo(uint64_t in_count, uint64_t in_offset)
      : count_(in_count),
        offset_(in_offset)
    { }
    uint64_t count_ = 0;
    uint64_t offset_ = 0;
  };

public:
  void initialize(
    ProxyType in_proxy, bool in_is_migratable, IndexT in_range,
    vt::HandlerType map_han, bool in_dense_start_with_zero
  );

  void makeSubHandles(bool initial = true);

  typename IndexT::DenseIndexType linearize(IndexT idx);

  NodeType getHomeNode(IndexT const& idx);

  IndexInfo fetchInfo(IndexT const& idx);

  void updateInfo(IndexT const& idx, IndexInfo info, NodeType home);

  IndexInfo resolveLocation(IndexT const& idx);

  void get(IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset);

  std::size_t getCount(IndexT const& idx, Lock l = Lock::Shared);

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

  T fetchOp(IndexT const& idx, Lock l, T ptr, int offset, MPI_Op op);

  bool isUniform() const { return uniform_size_; }

  template <typename Callable>
  void access(IndexT idx, Lock l, Callable fn, uint64_t offset);

  Handle<T, E, IndexT> addLocalIndex(IndexT index, uint64_t count);

  int getOrderedOffset(IndexT idx, NodeType home_node);

  void stageLocalIndex(IndexT index, uint64_t count);

  void migratedOutIndex(IndexT index);

  void migratedInIndex(IndexT index);

  void afterLB();

  void checkChanged(impl::ReduceLBMsg* msg);

  bool ready() const { return ready_; }

  uint64_t totalLocalCount() const;

  std::size_t getNumHandles() const;

  std::size_t getNumActiveHandles() const;

  void deleteHandle();

  std::size_t getNumDeletedHandles() const;

  std::size_t getCollectionExpected() const;

  void setCollectionExpected(std::size_t count);

  void destroy();

  static void destroyCollective(ProxyType proxy);

  template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
  static ProxyType construct(
    bool in_is_migratable, IndexT in_range, bool in_dense_start_with_zero
  );
  static ProxyType construct(
    bool in_is_migratable, IndexT in_range, bool in_dense_start_with_zero,
    vt::HandlerType map
  );

private:
  template <typename U>
  void waitForHandleReady(Handle<U,E> const& h);

protected:
  ProxyType proxy_;
  bool is_migratable_ = false;
  vt::HandlerType map_han_ = 0;
  IndexT range_ = {};
  std::unordered_map<IndexT, SubInfo> sub_handles_;
  std::unordered_map<IndexT, std::size_t> sub_handles_staged_;
  std::vector<IndexT> sub_layout_;
  std::vector<uint64_t> sub_prefix_;
  Handle<T, E> data_handle_;
  Handle<uint64_t, E> loc_handle_;
  Cache<IndexT> cache_;
  bool ready_ = false;
  bool mpi2_ = false;
  bool uniform_size_ = false;
  std::size_t count_if_uniform_ = 0;
  std::size_t collection_expected_count_ = 0;
  std::vector<IndexT> migrate_out_;
  std::vector<IndexT> migrate_in_;
  bool ordered_opt_ = true;
  std::unordered_map<IndexT, int> ordered_local_offset_;
  bool dense_start_with_zero_ = true;
  std::size_t deleted_count_ = 0;
};

}} /* end namespace vt::rdma */

#include "vt/rdmahandle/sub_handle.impl.h"

#endif /*INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H*/
