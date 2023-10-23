/*
//@HEADER
// *****************************************************************************
//
//                              task_collective.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_MESSAGING_TASK_COLLECTIVE_H
#define INCLUDED_VT_MESSAGING_TASK_COLLECTIVE_H

#include "vt/vrt/collection/manager.fwd.h"
#include "vt/objgroup/manager.h"
#include "vt/topos/location/manager.h"
#include "vt/pipe/pipe_manager.h"

#include <utility>

namespace vt::task {

using TaskIDType = uint64_t;

constexpr TaskIDType const no_task = 0;

template <typename Index>
struct Dependency {
  Index idx = {};
  TaskIDType id = no_task;
};

extern TaskIDType cur_id_;

template <typename Index>
struct TaskCollectiveManager;

template <typename Index>
struct TaskCollective {

  struct NewTaskTag {};

  TaskCollective() = default;

  operator TaskIDType() const { return id_; }

  TaskCollective(
    NewTaskTag,
    VirtualProxyType in_proxy,
    TaskCollectiveManager<Index>* in_manager
  ) : id_(cur_id_++),
      proxy_(in_proxy),
      manager_(in_manager)
  { }

  void add(Index idx, EpochType ep) {
    vt_debug_print(terse, gen, "add: idx={}, ep={}, id={}\n", idx, ep, id_);
    vtAssert(epochs_.find(idx) == epochs_.end(), "Must not exist");
    epochs_[idx] = ep;
  }

  void dependsOn(std::initializer_list<std::tuple<Index, TaskIDType>> deps) {
    vtAssert(context_ != 0, "Must be in a proper context to add dependency");
    deps_[*context_].insert(deps.begin(), deps.end());
  }

  void dependsOn(Index idx, TaskIDType id) {
    vtAssert(context_ != 0, "Must be in a proper context to add dependency");
    vt_debug_print(terse, gen, "dependsOn: idx={}, dep_idx={}, id={}\n", *context_, idx, id);
    deps_[*context_].insert(std::make_tuple(idx, id));
  }

  void setContext(Index const* idx) {
    context_ = idx;
  }

  TaskIDType getID() const { return id_; }

  void removeDependency(Index idx, Index dep_idx, TaskIDType tid) {
    vt_debug_print(terse, gen, "idx={}, dep_idx={}, tid={}\n", idx, dep_idx, tid);
    auto it = deps_.find(idx);
    vtAssertExpr(it != deps_.end());
    it->second.erase(it->second.find(std::make_tuple(dep_idx, tid)));
    if (it->second.size() == 0) {
      deps_.erase(it);
      done(idx);
    }
  }

  void done(Index idx) {
    auto iter = deps_.find(idx);

    // No dependencies, release immediately
    if (iter == deps_.end()) {
      vrt::collection::releaseEpochCollectionElm(proxy_, idx, epochs_[idx]);
    } else {
      auto const& dep_set = iter->second;
      for (auto const& [dep_idx, id] : dep_set) {
        auto dep_tc = manager_->getTaskCollective(id);
        if (auto i = dep_tc->epochs_.find(dep_idx); i != dep_tc->epochs_.end()) {
          auto dep_ep = i->second;
          theTerm()->addAction(
            dep_ep,
            [this,idx,dep_idx,id]{ removeDependency(idx, dep_idx, id); }
          );
        } else {
          manager_->getDepInfo(idx, dep_idx, id_, id);
        }
      }
    }
  }

  EpochType getEpochForIdx(Index idx) const {
    for (auto const& [i, e] : epochs_) {
      vt_debug_print(terse, gen, "getEpochForIdx: i={}, e={:x}\n", i, e);
    }
    vtAssertExpr(epochs_.find(idx) != epochs_.end());
    return epochs_.find(idx)->second;
  }

private:
  TaskIDType id_ = no_task;
  std::unordered_map<Index, EpochType> epochs_;
  Index const* context_ = nullptr;
  std::unordered_map<Index, std::set<std::tuple<Index, TaskIDType>>> deps_;
  VirtualProxyType proxy_ = no_vrt_proxy;
  TaskCollectiveManager<Index>* manager_ = nullptr;
};

template <typename Index>
struct TaskCollectiveManager {
  using ThisType = TaskCollectiveManager<Index>;
  using TaskPtrType = std::unique_ptr<TaskCollective<Index>>;

  static auto construct(bool in_local_tracking, VirtualProxyType in_col_proxy) {
    auto proxy = theObjGroup()->makeCollective<ThisType>("TCManager");
    proxy.get()->setProxy(proxy);
    proxy.get()->local_tracking_ = in_local_tracking;
    proxy.get()->col_proxy_ = in_col_proxy;
    return proxy;
  }

  TaskCollective<Index>* addTaskCollective(VirtualProxyType in_proxy) {
    auto tc = std::make_unique<task::TaskCollective<Index>>(
      typename task::TaskCollective<Index>::NewTaskTag{}, in_proxy, this
    );
    auto tc_raw = tc.get();
    auto tc_id = tc->getID();
    tasks_.emplace(tc_id, std::move(tc));
    return tc_raw;
  }

  TaskCollective<Index>* getTaskCollective(TaskIDType id) {
    if (auto it = tasks_.find(id); it != tasks_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  void depInfoHan(
    Index idx, Index dep_idx, TaskIDType id, TaskIDType dep_id,
    Callback<Index, Index, TaskIDType, TaskIDType, EpochType> cb
  ) {
    vt_debug_print(terse, gen, "depInfoHan: idx={}, dep_idx={}, id={}, dep_id={}\n", idx, dep_idx, id, dep_id);
    auto tc = getTaskCollective(dep_id);
    auto ep = tc->getEpochForIdx(dep_idx);
    cb.send(idx, dep_idx, id, dep_id, ep);
  }

  void depRecvInfoHan(
    Index idx, Index dep_idx, TaskIDType id, TaskIDType dep_id, EpochType ep
  ) {
    auto tc = getTaskCollective(id);
    theTerm()->addAction(
      ep, [=]{ tc->removeDependency(idx, dep_idx, dep_id); }
    );
  }

  void getDepInfo(Index idx, Index dep_idx, TaskIDType id, TaskIDType dep_id) {
    auto mapped_node = vrt::collection::getMappedNodeElm(col_proxy_, dep_idx);
    vt_debug_print(terse, gen, "getDepInfo: idx={}, dep_idx={}, id={}, dep_id={}, mapped_node={}\n", idx, dep_idx, id, dep_id, mapped_node);

    auto send_dep = [=](NodeType node) {
      auto const this_node = theContext()->getNode();
      auto cb = theCB()->makeSend<&ThisType::depRecvInfoHan>(proxy_[this_node]);
      vt_debug_print(terse, gen, "send_dep: dep_idx={}, node={}\n", dep_idx, node);
      proxy_[node].template send<&ThisType::depInfoHan>(idx, dep_idx, id, dep_id, cb);
    };

    if (local_tracking_) {
      auto lm = theLocMan()->getCollectionLM<Index>(col_proxy_);
      lm->getLocation(dep_idx, mapped_node, send_dep);
    } else {
      // Tracking on home processor
      send_dep(mapped_node);
    }
  }

  void setProxy(objgroup::proxy::Proxy<ThisType> proxy) {
    proxy_ = proxy;
  }

private:
  objgroup::proxy::Proxy<ThisType> proxy_;
  std::unordered_map<TaskIDType, TaskPtrType> tasks_;
  bool local_tracking_ = false;
  VirtualProxyType col_proxy_ = no_vrt_proxy;
};

} /* end namespace vt::task */

#endif /*INCLUDED_VT_MESSAGING_TASK_COLLECTIVE_H*/
