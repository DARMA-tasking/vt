/*
//@HEADER
// *****************************************************************************
//
//                              sub_handle.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_IMPL_H

#include "vt/config.h"
#include "vt/objgroup/manager.h"
#include "vt/collective/reduce/operators/default_msg.h"

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::initialize(
  ProxyType in_proxy, bool in_is_migratable, IndexT in_range, vt::HandlerType map_han,
  bool in_dense_start_with_zero
) {
  map_han_ = map_han;
  proxy_ = in_proxy;
  is_migratable_ = in_is_migratable;
  range_ = in_range;
  dense_start_with_zero_ = in_dense_start_with_zero;
  ordered_opt_ = ordered_opt_ and dense_start_with_zero_ and not is_migratable_;
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::makeSubHandles(bool initial) {
  using BaseIdxType = vt::index::BaseIndex;

  if (ordered_opt_) {
    auto this_node = theContext()->getNode();
    auto num_nodes = theContext()->getNumNodes();
    auto map_fn = auto_registry::getHandlerMap(map_han_);
    range_.foreach([&](IndexT cur_idx) {
      auto* cur_idx_ptr = static_cast<BaseIdxType*>(&cur_idx);
      auto* range_ptr = static_cast<BaseIdxType*>(&range_);
      auto home_node = map_fn(cur_idx_ptr, range_ptr, num_nodes);
      auto iter = sub_handles_staged_.find(cur_idx);
      if (home_node == this_node and iter != sub_handles_staged_.end()) {
        stageLocalIndex(iter->first, iter->second);
      }
    });
  } else {
    for (auto&& h : sub_handles_staged_) {
      stageLocalIndex(h.first, h.second);
    }
  }
  sub_handles_staged_.clear();

  auto const total = totalLocalCount();
  auto const num_local = sub_handles_.size();
  debug_print(
    rdma, node,
    "total={}, num_local={}, is_migratable_={}\n",
    total, num_local, is_migratable_
  );
  for (uint64_t i = 0; i < sub_prefix_.size(); i++) {
    debug_print(
      rdma, node,
      "i={} prefix={} layout={}\n",
      i, sub_prefix_[i], sub_layout_[i]
    );
  }
  data_handle_ = proxy_.template makeHandleRDMA<T>(total, false);
  waitForHandleReady(data_handle_);
  if (initial) {
    // Handle case when the local size is zero
    auto loc_len =
      num_local > 0 ? (is_migratable_ ? num_local * 4 : (num_local + 1) * 2) : 0;
    debug_print(
      rdma, node,
      "total={}, num_local={}, is_migratable_={}, loc_len={}\n",
      total, num_local, is_migratable_, loc_len
    );
    loc_handle_ = proxy_.template makeHandleRDMA<uint64_t>(loc_len, false);
    waitForHandleReady(loc_handle_);
    vtAssertExpr(sub_prefix_.size() == num_local);
    vtAssertExpr(sub_layout_.size() == num_local);
    if (loc_len > 0) {
      if (not is_migratable_) {
        loc_handle_.modifyExclusive([&](uint64_t* t, std::size_t) {
          uint64_t i = 0;
          for (i = 0; i < num_local * 2; i += 2) {
            t[i+0] = linearize(sub_layout_[i/2]);
            t[i+1] = sub_prefix_[i/2];
          }
          t[i+0] = 0;
          t[i+1] = sub_handles_[sub_layout_[sub_layout_.size()-1]].count_ + sub_prefix_[(i-2)/2];
        });
      } else {
        auto this_node = theContext()->getNode();
        loc_handle_.modifyExclusive([&](uint64_t* t, std::size_t) {
          uint64_t i = 0;
          for (i = 0; i < num_local * 4; i += 4) {
            t[i+0] = linearize(sub_layout_[i/4]);
            t[i+1] = sub_prefix_[i/4];
            t[i+2] = this_node;
            t[i+3] = sub_handles_[sub_layout_[i/4]].count_;
          }
        });
      }
    }
  }
  ready_ = true;
}

template <typename T, HandleEnum E, typename IndexT>
typename IndexT::DenseIndexType
SubHandle<T,E,IndexT>::linearize(IndexT idx) {
  debug_print(
    rdma, node,
    "linearize: idx={}, range={}\n",
    idx, range_
  );
  return vt::mapping::linearizeDenseIndexRowMajor(&idx, &range_);
}

template <typename T, HandleEnum E, typename IndexT>
NodeType SubHandle<T,E,IndexT>::getHomeNode(IndexT const& idx) {
  using BaseIdxType = vt::index::BaseIndex;
  auto fn = auto_registry::getHandlerMap(map_han_);
  auto const num_nodes = theContext()->getNumNodes();
  auto idx_p = idx;
  auto* cur = static_cast<BaseIdxType*>(&idx_p);
  auto* range = static_cast<BaseIdxType*>(&range_);
  return fn(cur, range, num_nodes);
}

template <typename T, HandleEnum E, typename IndexT>
int SubHandle<T,E,IndexT>::getOrderedOffset(IndexT idx, NodeType home_node) {
  auto iter = ordered_local_offset_.find(idx);
  int found_offset = 0;
  if (iter == ordered_local_offset_.end()) {
    using BaseIdxType = vt::index::BaseIndex;
    auto num_nodes = theContext()->getNumNodes();
    auto map_fn = auto_registry::getHandlerMap(map_han_);
    int offset = 0;
    range_.foreach([&](IndexT cur_idx) {
      auto* cur_idx_ptr = static_cast<BaseIdxType*>(&cur_idx);
      auto* range_ptr = static_cast<BaseIdxType*>(&range_);
      auto map_node = map_fn(cur_idx_ptr, range_ptr, num_nodes);
      if (home_node == map_node) {
        if (cur_idx == idx) {
          found_offset = offset;
        }
        offset++;
      }
    });
    ordered_local_offset_[idx] = found_offset;
  } else {
    found_offset = iter->second;
  }
  return found_offset;
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::updateInfo(
  IndexT const& idx, IndexInfo info, NodeType home
) {
  vtAssertExpr(not ordered_opt_);
  vtAssertExpr(is_migratable_);
  auto this_node = theContext()->getNode();
  auto home_size = loc_handle_.getCount(home);
  debug_print(
    rdma, node,
    "updateInfo: idx={}, this_node={}, home={}, home_size={}\n",
    idx, this_node, home, home_size
  );
  auto lin_idx = linearize(idx);
  auto ptr = std::make_unique<uint64_t[]>(home_size);
  // These can probably be relaxed to Shared locks because lin_idx is never
  // modified
  loc_handle_.get(home, &ptr[0], home_size, 0, Lock::Exclusive);
  for (uint64_t i = 0; i < home_size; i += 4) {
    debug_print_verbose(
      rdma, node,
      "updateInfo: idx={}, node={}, home={}, ptr[{}]={},{},{},{}\n",
      idx, this_node, home, i, ptr[i+0], ptr[i+1], ptr[i+2], ptr[i+3]
    );
    if (ptr[i] == static_cast<uint64_t>(lin_idx)) {
      debug_print_verbose(
        rdma, node,
        "updateInfo: idx={}, node={}, home={}, "
        "ptr[{}]={},{},{},{}\n",
        idx, this_node, home, i,
        ptr[i+0], ptr[i+1], ptr[i+2], ptr[i+3]
      );
      int offset = static_cast<int>(i);
      auto pptr = std::make_unique<uint64_t[]>(2);
      pptr[0] = info.getOffset();
      pptr[1] = info.getNode();
      loc_handle_.put(home, &pptr[0], 2, offset + 1, Lock::Exclusive);
    }
  }
}

template <typename T, HandleEnum E, typename IndexT>
IndexInfo SubHandle<T,E,IndexT>::fetchInfo(IndexT const& idx) {
  auto this_node = theContext()->getNode();
  auto home_node = getHomeNode(idx);
  if (ordered_opt_) {
    auto offset = getOrderedOffset(idx, home_node);
    if (not is_migratable_) {
      auto ptr = std::make_unique<uint64_t[]>(4);
      loc_handle_.get(home_node, &ptr[0], 4, offset*2, Lock::Exclusive);
      auto lin_idx = linearize(idx);
      debug_print_verbose(
        rdma, node,
        "fetchInfo: ordered: offset={}, idx={}, node={}, home={}, "
        "ptr={},{}, {},{}\n",
        offset, idx, this_node, home_node, ptr[0], ptr[1], ptr[2], ptr[3]
      );
      vtAssertExpr(ptr[0] == static_cast<uint64_t>(lin_idx));
      return IndexInfo(home_node, ptr[1], ptr[3]-ptr[1]);
    } else {
      auto ptr = std::make_unique<uint64_t[]>(4);
      loc_handle_.get(home_node, &ptr[0], 4, offset*4, Lock::Exclusive);
      auto lin_idx = linearize(idx);
      debug_print_verbose(
        rdma, node,
        "fetchInfo: ordered: offset={}, idx={}, node={}, home={},"
        " ptr={},{},{},{}\n",
        offset, idx, this_node, home_node, ptr[0], ptr[1], ptr[2], ptr[3]
      );
      vtAssertExpr(ptr[0] == static_cast<uint64_t>(lin_idx));
      return IndexInfo(ptr[2], ptr[1], ptr[3]);
    }
  } else {
    auto home_size = loc_handle_.getCount(home_node);
    debug_print(
      rdma, node,
      "fetchInfo: idx={}, this_node={}, home_node={}, home_size={}\n",
      idx, this_node, home_node, home_size
    );
    auto lin_idx = linearize(idx);
    auto ptr = std::make_unique<uint64_t[]>(home_size);
    loc_handle_.get(home_node, &ptr[0], home_size, 0, Lock::Exclusive);
    if (not is_migratable_) {
      for (uint64_t i = 0; i < home_size; i += 2) {
        debug_print_verbose(
          rdma, node,
          "fetchInfo: idx={}, node={}, home={}, ptr[{}]={},{}\n",
          idx, this_node, home_node, i, ptr[i+0], ptr[i+1]
        );
        if (ptr[i] == static_cast<uint64_t>(lin_idx)) {
          debug_print_verbose(
            rdma, node,
            "fetchInfo: idx={}, node={}, home={}, ptr[{}]={},{}, {},{}\n",
            idx, this_node, home_node, i, ptr[i+0], ptr[i+1],
            ptr[i+2], ptr[i+3]
          );
          return IndexInfo(home_node, ptr[i+1], ptr[i+3]-ptr[i+1]);
        }
      }
    } else {
      for (uint64_t i = 0; i < home_size; i += 4) {
        debug_print_verbose(
          rdma, node,
          "fetchInfo: idx={}, node={}, home={}, ptr[{}]={},{},{},{}\n",
          idx, this_node, home_node, i, ptr[i+0], ptr[i+1], ptr[i+2], ptr[i+3]
        );
        if (ptr[i] == static_cast<uint64_t>(lin_idx)) {
          debug_print_verbose(
            rdma, node,
            "fetchInfo: idx={}, node={}, home={}, "
            "ptr[{}]={},{},{},{}\n",
            idx, this_node, home_node, i,
            ptr[i+0], ptr[i+1], ptr[i+2], ptr[i+3]
          );
          return IndexInfo(ptr[i+2], ptr[i+1], ptr[i+3]);
        }
      }
    }
  }
  vtAssert(false, "Could not find location info");
  return IndexInfo{};
}

template <typename T, HandleEnum E, typename IndexT>
IndexInfo SubHandle<T,E,IndexT>::resolveLocation(IndexT const& idx) {
  auto iter = sub_handles_.find(idx);
  if (iter != sub_handles_.end()) {
    auto const this_node = vt::theContext()->getNode();
    return IndexInfo(this_node, iter->second.offset_, iter->second.count_);
  } else {
    if (cache_.hasIndex(idx)) {
      return cache_.getInfo(idx);
    } else {
      auto info = fetchInfo(idx);
      cache_.saveInfo(idx, info);
      return info;
    }
  }
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::get(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  data_handle_.get(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
std::size_t SubHandle<T,E,IndexT>::getCount(IndexT const& idx, Lock) {
  if (uniform_size_) {
    return count_if_uniform_;
  } else {
    auto info = resolveLocation(idx);
    return info.getCount();
  }
}

template <typename T, HandleEnum E, typename IndexT>
RequestHolder SubHandle<T,E,IndexT>::rget(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  // @todo: make the location lookup async
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.rget(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
RequestHolder SubHandle<T,E,IndexT>::rput(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  // @todo: make the location lookup async
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.rput(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::put(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  data_handle_.put(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
RequestHolder SubHandle<T,E,IndexT>::raccum(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset,
  MPI_Op op
) {
  // @todo: make the location lookup async
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.raccum(node, ptr, len, offset + chunk_offset, op, l);
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::accum(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset,
  MPI_Op op
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  data_handle_.accum(node, ptr, len, offset + chunk_offset, op, l);
}

template <typename T, HandleEnum E, typename IndexT>
T SubHandle<T,E,IndexT>::fetchOp(
  IndexT const& idx, Lock l, T ptr, int offset, MPI_Op op
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.fetchOp(node, ptr, offset + chunk_offset, op, l);
}

template <typename T, HandleEnum E, typename IndexT>
template <typename Callable>
void SubHandle<T,E,IndexT>::access(
  IndexT idx, Lock l, Callable fn, uint64_t offset
) {
  auto iter = sub_handles_.find(idx);
  vtAssert(iter != sub_handles_.end(), "Index must be local here");
  auto local_offset  = iter->second.offset_;
  data_handle_.access(l, fn, offset + local_offset);
}

template <typename T, HandleEnum E, typename IndexT>
Handle<T, E, IndexT> SubHandle<T,E,IndexT>::addLocalIndex(
  IndexT index, uint64_t count
) {
  sub_handles_staged_[index] = count;
  return Handle<T,E,IndexT>{
    typename Handle<T,E,IndexT>::IndexTagType{},
    proxy_.getProxy(), index, count, 0
  };
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::stageLocalIndex(
  IndexT index, uint64_t count
) {
  debug_print(
    rdma, node,
    "addLocalIndex: idx={}, count={}, range={}\n",
    index, count, range_
  );
  sub_layout_.push_back(index);
  if (sub_prefix_.size() > 0) {
    vtAssertExpr(sub_layout_.size() >= 2);
    // Compute a prefix as elements are added
    auto last_idx = sub_layout_[sub_layout_.size()-2];
    auto last_count_iter = sub_handles_.find(last_idx);
    vtAssertExpr(last_count_iter != sub_handles_.end());
    auto last_count = last_count_iter->second.count_;
    sub_prefix_.push_back(sub_prefix_[sub_prefix_.size()-1] + last_count);
  } else {
    sub_prefix_.push_back(0);
  }
  auto const offset = sub_prefix_[sub_prefix_.size()-1];
  sub_handles_[index] = SubInfo(count, offset);
  if (uniform_size_) {
    count_if_uniform_ = static_cast<std::size_t>(count);
  }
}

template <typename T, HandleEnum E, typename IndexT>
uint64_t SubHandle<T,E,IndexT>::totalLocalCount() const {
  uint64_t total = 0;
  for (auto&& elm : sub_handles_) {
    total += elm.second.count_;
  }
  return total;
}

template <typename T, HandleEnum E, typename IndexT>
std::size_t SubHandle<T,E,IndexT>::getNumHandles() const {
  return sub_handles_staged_.size();
}

template <typename T, HandleEnum E, typename IndexT>
std::size_t SubHandle<T,E,IndexT>::getNumActiveHandles() const {
  return sub_handles_.size();
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::deleteHandle() {
  deleted_count_++;
}

template <typename T, HandleEnum E, typename IndexT>
std::size_t SubHandle<T,E,IndexT>::getNumDeletedHandles() const {
  return deleted_count_;
}

template <typename T, HandleEnum E, typename IndexT>
template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
/*static*/ typename SubHandle<T,E,IndexT>::ProxyType
SubHandle<T,E,IndexT>::construct(
  bool in_is_migratable, IndexT in_range, bool in_dense_start_with_zero
) {
  auto map_han = auto_registry::makeAutoHandlerMap<IndexT, map_fn>();
  return construct(in_is_migratable, in_range, in_dense_start_with_zero, map_han);
}

template <typename T, HandleEnum E, typename IndexT>
/*static*/ typename SubHandle<T,E,IndexT>::ProxyType
SubHandle<T,E,IndexT>::construct(
  bool in_is_migratable, IndexT in_range, bool in_dense_start_with_zero,
  vt::HandlerType map_han
) {
  auto proxy = vt::theObjGroup()->makeCollective<SubHandle<T,E,IndexT>>();
  proxy.get()->initialize(
    proxy, in_is_migratable, in_range, map_han, in_dense_start_with_zero
  );
  return proxy;
}

template <typename T, HandleEnum E, typename IndexT>
template <typename U>
void SubHandle<T,E,IndexT>::waitForHandleReady(Handle<U,E> const& h) {
  theSched()->runSchedulerWhile([&h] { return not h.ready(); });
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::destroy() {
  proxy_.destroyHandleRDMA(data_handle_);
  proxy_.destroyHandleRDMA(loc_handle_);
}

template <typename T, HandleEnum E, typename IndexT>
/*static*/ void SubHandle<T,E,IndexT>::destroyCollective(ProxyType proxy) {
  proxy.get()->destroy();
  theObjGroup()->destroyCollective(proxy);
}

template <typename T, HandleEnum E, typename IndexT>
std::size_t SubHandle<T,E,IndexT>::getCollectionExpected() const {
  return collection_expected_count_;
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::setCollectionExpected(std::size_t count) {
  collection_expected_count_ = count;
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::migratedOutIndex(IndexT index) {
  vtAssertExpr(is_migratable_);
  debug_print(
    rdma, node,
    "migratedOutIndex: idx={}\n", index
  );
  migrate_out_.push_back(index);
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::migratedInIndex(IndexT index) {
  vtAssertExpr(is_migratable_);
  debug_print(
    rdma, node,
    "migratedInIndex: idx={}\n", index
  );
  migrate_in_.push_back(index);
}

namespace impl {

struct ReduceLBMsg : vt::collective::ReduceTMsg<bool> {
  explicit ReduceLBMsg(bool changed)
    : vt::collective::ReduceTMsg<bool>(changed)
  { }
};

} /* end namespace impl */

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::afterLB() {
  using ThisType = SubHandle<T,E,IndexT>;
  vtAssertExpr(is_migratable_);
  bool local_changed = migrate_in_.size() != 0 or migrate_out_.size() != 0;
  auto cb = theCB()->makeBcast<
    ThisType,impl::ReduceLBMsg,&ThisType::checkChanged
  >(proxy_);
  auto epoch = theTerm()->makeEpochCollective();
  theMsg()->pushEpoch(epoch);
  auto msg = makeMessage<impl::ReduceLBMsg>(local_changed);
  proxy_.template reduce<collective::OrOp<bool>>(msg.get(),cb);
  theMsg()->popEpoch(epoch);
  theTerm()->finishedEpoch(epoch);

  runSchedulerThrough(epoch);
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::checkChanged(impl::ReduceLBMsg* msg) {
  auto global_changed = msg->getVal();

  debug_print(
    rdma, node,
    "checkChanged: global_changed={}\n", global_changed
  );

  // Must re-configure all windows
  if (global_changed) {
    std::vector<IndexT> cur_handles;
    for (auto&& h : sub_handles_) {
      cur_handles.push_back(h.first);
    }

    debug_print(
      rdma, node,
      "checkChanged: cur_handles={}, in={}, out={}\n",
      cur_handles.size(), migrate_in_.size(), migrate_out_.size()
    );

    std::vector<IndexT> all_handles;
    // Union current with migrated in
    std::set_union(
      cur_handles.begin(), cur_handles.end(),
      migrate_in_.begin(), migrate_in_.end(),
      std::back_inserter(all_handles)
    );

    debug_print(
      rdma, node,
      "checkChanged: all_handles={}, in={}, out={}\n",
      all_handles.size(), migrate_in_.size(), migrate_out_.size()
    );

    std::vector<IndexT> new_handles;
    std::sort(all_handles.begin(), all_handles.end());
    std::sort(migrate_out_.begin(), migrate_out_.end());
    // Minus off the ones migrated out
    std::set_difference(
      all_handles.begin(), all_handles.end(),
      migrate_out_.begin(), migrate_out_.end(),
      std::inserter(new_handles, new_handles.begin())
    );

    debug_print(
      rdma, node,
      "checkChanged: new_handles={}, in={}, out={}\n",
      new_handles.size(), migrate_in_.size(), migrate_out_.size()
    );

    debug_print(
      rdma, node,
      "checkChanged: in.size()={}, out.size()={}\n",
      migrate_in_.size(), migrate_out_.size()
    );

    // Now, we need the sizes to re-create the handles that belong here
    std::unordered_map<IndexT, std::size_t> new_handles_sized;
    for (auto&& h : new_handles) {
      debug_print(
        rdma, node,
        "checkChanged: fetching size: idx={}\n", h
      );

      // Fetch the size, which may be remote if it migrated
      new_handles_sized[h] = getCount(h);
    }

    debug_print(
      rdma, node,
      "checkChanged: fetched all sizes\n"
    );

    // Fetch all the data for the new handles into buffers
    // @todo: possibly we could do a local memcpy for the purely local
    // transfers, but it would be very difficult
    std::unordered_map<IndexT, std::unique_ptr<T[]>> idx_data;
    std::vector<RequestHolder> reqs;
    for (auto&& h : new_handles_sized) {
      idx_data[h.first] = std::make_unique<T[]>(h.second);
      reqs.emplace_back(
        rget(h.first, vt::Lock::Shared, &idx_data[h.first][0], h.second, 0)
      );
    }

    // Wait for all the fetches to complete
    for (auto&& r : reqs) {
      r.wait();
    }
    reqs.clear();

    // All must finish before we start destroying the hold sub-handle info
    theCollective()->barrier();

    // Clear out all the incremental building data structures
    sub_handles_.clear();
    sub_layout_.clear();
    sub_prefix_.clear();
    if (ordered_opt_) {
      ordered_local_offset_.clear();
    }

    // New we have everything to re-build the sub-handles
    for (auto&& h : new_handles_sized) {
      addLocalIndex(h.first, h.second);
    }

    debug_print(
      rdma, node,
      "checkChanged: new_handles_sized={}, staged={}\n",
      new_handles_sized.size(), sub_handles_staged_.size()
    );

    // Destroy the old data handle
    proxy_.destroyHandleRDMA(data_handle_);

    // Make the new sub-handles, with the new local indices now mapped here
    makeSubHandles(false);

    // Invalidate the cache, since handles have migrated and the cached
    // locations may not be valid anymore
    cache_.invalidate();

    // Make sure everyone has invalidated, as to not get bad values
    theCollective()->barrier();

    // Update the locations for all the handles since they are been
    // re-configured
    for (auto&& m : new_handles_sized) {
      auto home = getHomeNode(m.first);
      // This resolveLocation better be a local operation!
      auto info = resolveLocation(m.first);
      updateInfo(m.first, info, home);
    }

    // Make sure everyone has updated their location data
    theCollective()->barrier();

    // Put all the old data where it goes
    for (auto&& h : new_handles_sized) {
      reqs.emplace_back(
        rput(h.first, vt::Lock::Shared, &idx_data[h.first][0], h.second, 0)
      );
    }

    // Wait for all the puts to complete
    for (auto&& r : reqs) {
      r.wait();
    }
    reqs.clear();

    // Clear the migrate buffers
    migrate_in_.clear();
    migrate_out_.clear();

    // Make sure all the puts are globally complete before we exit this block
    theCollective()->barrier();
  }
}


}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_IMPL_H*/
