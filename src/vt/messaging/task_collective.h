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

/// Task ID type to identify a collective task
using TaskIDType = uint64_t;

/// No task sentinel
constexpr TaskIDType const no_task = 0;

template <typename Index>
struct TaskCollectiveManager;

/**
 * \struct TaskCollective
 *
 * \brief A collective task that runs across a set of indices
 */
template <typename Index>
struct TaskCollective {

  /// Tag to create a new collective task
  struct NewTaskTag {};

  TaskCollective() = default;

  /**
   * \internal \brief Create a new collective task
   *
   * \param[in] NewTaskTag tag
   * \param[in] in_proxy the collection proxy
   * \param[in] in_manager collective task manager
   */
  TaskCollective(
    NewTaskTag,
    VirtualProxyType in_proxy,
    TaskCollectiveManager<Index>* in_manager
  ) : id_(in_manager->getNextID()),
      proxy_(in_proxy),
      manager_(in_manager)
  { }

  /**
   * \internal \brief Add a new task for an index
   *
   * \param[in] idx the index
   * \param[in] ep the rooted, dependent epoch
   */
  void addTask(Index idx, EpochType ep) {
    vt_debug_print(
      normal, gen,
      "add: idx={}, ep={}, id={}\n", idx, ep, id_
    );

    vtAssert(epochs_.find(idx) == epochs_.end(), "Must not exist");
    epochs_[idx] = ep;
  }

  /**
   * \brief Add a set of dependencies for this task
   *
   * \param[in] deps set of dependencies
   */
  void dependsOn(std::initializer_list<std::tuple<Index, TaskCollective<Index>*>> deps) {
    vtAssert(context_ != 0, "Must be in a proper context to add dependency");

    for (auto const& [idx, tc] : deps) {
      deps_[*context_].emplace(idx, tc->getID());
    }
  }

  /**
   * \brief Add a dependency for a task
   *
   * \param[in] idx the index it depends on
   * \param[in] tc the task it depends on
   */
  void dependsOn(Index idx, TaskCollective<Index>* tc) {
    vtAssert(context_ != 0, "Must be in a proper context to add dependency");

    vt_debug_print(
      normal, gen,
      "dependsOn: idx={}, dep_idx={}, id={}\n",
      *context_, idx, tc->getID()
    );

    deps_[*context_].emplace(idx, tc->getID());
  }

  /**
   * \internal \brief Set context for the task collective
   *
   * \param[in] idx the current index running its task
   */
  void setContext(Index const* idx) {
    context_ = idx;
  }

  /**
   * \brief Get the underlying ID for the task collective
   *
   * \return the ID
   */
  TaskIDType getID() const { return id_; }

  /**
   * \brief Remove a dependency
   *
   * \param[in] idx the index
   * \param[in] dep_idx the index it depends on
   * \param[in] tid the task collective it depends on
   */
  void removeDependency(Index idx, Index dep_idx, TaskIDType tid) {
    vt_debug_print(terse, gen, "idx={}, dep_idx={}, tid={}\n", idx, dep_idx, tid);
    auto it = deps_.find(idx);
    vtAssertExpr(it != deps_.end());
    it->second.erase(it->second.find(std::make_tuple(dep_idx, tid)));
    if (it->second.size() == 0) {
      deps_.erase(it);
      checkDone(idx);
    }
  }

  /**
   * \brief Check if a task is ready to run
   *
   * \param[in] idx the index to check
   *
   * \return whether it is ready to run
   */
  bool checkDone(Index idx) {
    auto iter = deps_.find(idx);

    // No dependencies, release immediately
    if (iter == deps_.end()) {
      vt_debug_print(
        normal, gen,
        "checkDone: idx={}, epochs_[idx]={:x}\n",
        idx, epochs_[idx]
      );

      if constexpr (std::is_same_v<Index, NodeType>) {
        theSched()->fullyReleaseEpoch(epochs_[idx]);
      } else {
        vrt::collection::fullyReleaseEpoch(proxy_, idx, epochs_[idx]);
      }
      return true;
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
    return false;
  }

  /**
   * \brief Get the epoch for a given index
   *
   * \param[in] idx the index
   *
   * \return the epoch
   */
  EpochType getEpochForIdx(Index idx) const {
    vtAssert(epochs_.find(idx) != epochs_.end(), "Epoch must exist for index");
    return epochs_.find(idx)->second;
  }

private:
  /// The ID for the task collective
  TaskIDType id_ = no_task;
  /// The indices and epochs for each task
  std::unordered_map<Index, EpochType> epochs_;
  /// The current index context
  Index const* context_ = nullptr;
  /// The dependencies for each index
  std::unordered_map<Index, std::set<std::tuple<Index, TaskIDType>>> deps_;
  /// The underlying collection proxy
  VirtualProxyType proxy_ = no_vrt_proxy;
  /// The task collective manager
  TaskCollectiveManager<Index>* manager_ = nullptr;
};

/**
 * \struct TaskCollectiveManager
 *
 * \brief Manager for collective tasks
 *
 */
template <typename Index>
struct TaskCollectiveManager {
  using ThisType = TaskCollectiveManager<Index>;
  using TaskPtrType = std::unique_ptr<TaskCollective<Index>>;
  using CallbackType = Callback<
    Index, std::vector<std::tuple<Index, TaskIDType, TaskIDType, EpochType>>
  >;

  /**
   * \brief Construct the objgroup for managing collective tasks
   *
   * \param[in] in_local_tracking whether the collection uses local tracking
   * \param[in] in_col_proxy the collection proxy
   *
   * \return the objgroup proxy
   */
  static auto construct(bool in_local_tracking, VirtualProxyType in_col_proxy) {
    auto proxy = theObjGroup()->makeCollective<ThisType>("TCManager");
    proxy.get()->setProxy(proxy);
    proxy.get()->local_tracking_ = in_local_tracking;
    proxy.get()->col_proxy_ = in_col_proxy;
    return proxy;
  }

  /**
   * \brief Get the next ID for a new task collective
   *
   * \return a new ID
   */
  TaskIDType getNextID() { return cur_id_++; }

  /**
   * \brief Add a new collective task
   *
   * \param[in] in_proxy the collection proxy
   *
   * \return the task collective
   */
  TaskCollective<Index>* addTaskCollective(VirtualProxyType in_proxy) {
    auto tc = std::make_unique<task::TaskCollective<Index>>(
      typename task::TaskCollective<Index>::NewTaskTag{}, in_proxy, this
    );
    auto tc_raw = tc.get();
    auto tc_id = tc->getID();
    tasks_.emplace(tc_id, std::move(tc));
    return tc_raw;
  }

  /**
   * \brief Get a task collective from the ID
   *
   * \param[in] id the ID
   *
   * \return the task collective
   */
  TaskCollective<Index>* getTaskCollective(TaskIDType id) {
    if (auto it = tasks_.find(id); it != tasks_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  /**
   * \brief Fulfill dependency requests and send out any pending dependency
   * requests
   */
  void dispatchWork() {
    wait_iter_++;

    // Send out pending dependency requests
    for (auto const& [dep_idx, vec] : pending_deps_) {
      getDepInfoImpl(dep_idx, vec);
    }
    pending_deps_.clear();

    // Fulfill incoming requests now that we have all the task information
    for (auto const& [dep_idx, vec] : pending_lookups_) {
      for (auto const& [lookups, cb] : vec) {
        depInfoHan(dep_idx, lookups, cb, wait_iter_);
      }
    }
    pending_lookups_.clear();
  }

  /**
   * \internal \brief Request dependency info, buffers until ready to group and
   * send out
   *
   * \param[in] idx The requesting index
   * \param[in] dep_idx The dependency index
   * \param[in] id The requesting task collective
   * \param[in] dep_id The dependency task collective
   */
  void getDepInfo(Index idx, Index dep_idx, TaskIDType id, TaskIDType dep_id) {
    pending_deps_[dep_idx].emplace_back(DepInfo{idx, id, dep_id});
  }

protected:
  /**
   * \struct DepInfo
   *
   * \brief \c DepInfo holder
   */
  struct DepInfo {
    using isByteCopyable = std::true_type;

    Index idx;                  /**< Requesting index */
    TaskIDType id;              /**< The task collective for requesting index */
    TaskIDType dep_id;          /**< The task collective to the dependency */
  };

  /**
   * \brief Handle a dependency info request
   *
   * \param[in] dep_idx the dependency index
   * \param[in] vec a set of dependencies for this index
   * \param[in] cb callback to send back epochs
   * \param[in] wait_iter the wait collective iteration
   */
  void depInfoHan(
    Index dep_idx, std::vector<DepInfo> const& vec, CallbackType cb,
    int64_t wait_iter
  ) {
    if (wait_iter == wait_iter_) {
      std::vector<std::tuple<Index, TaskIDType, TaskIDType, EpochType>> result;
      for (auto const& [idx, id, dep_id] : vec) {
        auto tc =getTaskCollective(dep_id);
        auto ep = tc->getEpochForIdx(dep_idx);
        result.emplace_back(idx, id, dep_id, ep);
      }
      cb.send(dep_idx, std::move(result));
    } else {
      pending_lookups_[dep_idx].emplace_back(vec, cb);
    }
  }

  /**
   * \internal \brief Callback function handler for receiving dependency
   * information
   *
   * \param[in] dep_idx The index dependent on (all from the same target node)
   * \param[in] res The result vector
   */
  void depRecvInfoHan(
    Index dep_idx,
    std::vector<std::tuple<Index, TaskIDType, TaskIDType, EpochType>> const& res
  ) {
    for (auto [idx, id, dep_id, ep] : res) {
      vt_debug_print(
        normal, gen,
        "depRecvInfoHan: idx={}, dep_idx={}, id={}, dep_id={}, ep={:x}\n",
        idx, dep_idx, id, dep_id, ep
      );

      auto tc = getTaskCollective(id);
      theTerm()->addAction(
        ep, [=]{ tc->removeDependency(idx, dep_idx, dep_id); }
      );
    }
  }

  void getDepInfoImpl(Index dep_idx, std::vector<DepInfo> const& vec) {
    NodeType mapped_node = uninitialized_destination;

    if constexpr (std::is_same_v<Index, NodeType>) {
      mapped_node = dep_idx;
    } else {
      mapped_node = vrt::collection::getMappedNodeElm(col_proxy_, dep_idx);
    }

    vt_debug_print(
      terse, gen,
      "getDepInfoImpl: dep_idx={}\n",
      dep_idx
    );

    auto send_dep = [=](NodeType node) {
      auto const this_node = theContext()->getNode();
      auto cb = theCB()->makeSend<&ThisType::depRecvInfoHan>(proxy_[this_node]);
      vt_debug_print(terse, gen, "send_dep: dep_idx={}, node={}\n", dep_idx, node);
      proxy_[node].template send<&ThisType::depInfoHan>(dep_idx, vec, cb, wait_iter_);
    };

    if constexpr (std::is_same_v<Index, NodeType>) {
      // Node-based chain handling
      send_dep(mapped_node);
    } else {
      // Collection handling
      if (local_tracking_) {
        // Fetch the current location of the element
        auto lm = theLocMan()->getCollectionLM<Index>(col_proxy_);
        lm->getLocation(dep_idx, mapped_node, send_dep);
      } else {
        // Tracking on home processor, use the mapped node
        send_dep(mapped_node);
      }
    }
  }

  /**
   * \internal \brief Set the proxy for the objgroup
   *
   * \param[in] proxy the proxy
   */
  void setProxy(objgroup::proxy::Proxy<ThisType> proxy) {
    proxy_ = proxy;
  }

private:
  /// The objgroup proxy
  objgroup::proxy::Proxy<ThisType> proxy_;
  /// Collective tasks map
  std::unordered_map<TaskIDType, TaskPtrType> tasks_;
  /// Whether we are using local tracking
  bool local_tracking_ = false;
  /// The underlying collection proxy
  VirtualProxyType col_proxy_ = no_vrt_proxy;
  /// Pending dependency requests from other nodes
  std::unordered_map<
    Index, std::vector<std::tuple<std::vector<DepInfo>, CallbackType>>
  > pending_lookups_;
  /// Pending dependencies to resolve, waiting for all tasks to be created
  std::unordered_map<Index, std::vector<DepInfo>> pending_deps_;
  /// The current wait iteration
  int64_t wait_iter_ = 0;
  /// The next task collective ID
  TaskIDType cur_id_ = 1;
};

} /* end namespace vt::task */

#endif /*INCLUDED_VT_MESSAGING_TASK_COLLECTIVE_H*/
