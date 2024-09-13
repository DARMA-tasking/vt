/*
//@HEADER
// *****************************************************************************
//
//                               rabenseifner.cc
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

#include "vt/collective/reduce/allreduce/rabenseifner.h"
#include "vt/configs/error/config_assert.h"

namespace vt::collective::reduce::allreduce {

Rabenseifner::Rabenseifner(
  detail::StrongVrtProxy proxy, detail::StrongGroup group, size_t num_elems)
  : collection_proxy_(proxy.get()),
    local_num_elems_(num_elems),
    nodes_(theGroup()->GetGroupNodes(group.get())),
    num_nodes_(nodes_.size()),
    this_node_(theContext()->getNode()),
    num_steps_(static_cast<int32_t>(std::log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_) {

  auto const is_default_group = theGroup()->isGroupDefault(group.get());
  if (not is_default_group) {
    auto it = std::find(nodes_.begin(), nodes_.end(), theContext()->getNode());
    vtAssert(it != nodes_.end(), "This node was not found in group nodes!");

    this_node_ = it - nodes_.begin();
  }

  is_even_ = this_node_ % 2 == 0;
  is_part_of_adjustment_group_ = this_node_ < (2 * nprocs_rem_);
  if (is_part_of_adjustment_group_) {
    if (is_even_) {
      vrt_node_ = this_node_ / 2;
    } else {
      vrt_node_ = -1;
    }
  } else {
    vrt_node_ = this_node_ - nprocs_rem_;
  }

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner (this={}): proxy={:x} proxy_={} local_num_elems={}\n",
    print_ptr(this), proxy.get(), proxy_.getProxy(), local_num_elems_);
}

Rabenseifner::Rabenseifner(
  detail::StrongGroup group)
  : group_(group.get()),
    local_num_elems_(1),
    nodes_(theGroup()->GetGroupNodes(group.get())),
    num_nodes_(nodes_.size()),
    this_node_(theContext()->getNode()),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_) {
  std::string nodes_info;
  for (auto& node : nodes_) {
    nodes_info += fmt::format("{} ", node);
  }
  auto const is_default_group = theGroup()->isGroupDefault(group_);
  auto const in_group = theGroup()->inGroup(group_);
  auto const is_part_of_allreduce =
    (not is_default_group and in_group) or
    is_default_group;

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner: is_default_group={} is_part_of_allreduce={} num_nodes_={} "
    "Nodes:[{}]\n",
    is_default_group, is_part_of_allreduce, num_nodes_, nodes_info);

  if (not is_default_group and in_group) {
    auto it = std::find(nodes_.begin(), nodes_.end(), theContext()->getNode());
    vtAssert(it != nodes_.end(), "This node was not found in group nodes!");

    // index in group list
    this_node_ = it - nodes_.begin();
  }

  // We collectively create this Reducer, so it's possible that not all Nodes are part of it
  if (is_part_of_allreduce) {
    is_even_ = this_node_ % 2 == 0;
    is_part_of_adjustment_group_ = this_node_ < (2 * nprocs_rem_);
    if (is_part_of_adjustment_group_) {
      if (is_even_) {
        vrt_node_ = this_node_ / 2;
      } else {
        vrt_node_ = -1;
      }
    } else {
      vrt_node_ = this_node_ - nprocs_rem_;
    }
  }
}

Rabenseifner::Rabenseifner(detail::StrongObjGroup objgroup)
  : objgroup_proxy_(objgroup.get()),
    local_num_elems_(1),
    nodes_(theGroup()->GetGroupNodes(default_group)),
    num_nodes_(nodes_.size()),
    this_node_(theContext()->getNode()),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_) {
  std::string nodes_info;
  for (auto& node : nodes_) {
    nodes_info += fmt::format("{} ", node);
  }

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner: is_default_group={} is_part_of_allreduce={} num_nodes_={} "
    "Nodes:[{}]\n",
    true, true, num_nodes_, nodes_info);

  // We collectively create this Reducer, so it's possible that not all Nodes are part of it
  is_even_ = this_node_ % 2 == 0;
  is_part_of_adjustment_group_ = this_node_ < (2 * nprocs_rem_);
  if (is_part_of_adjustment_group_) {
    if (is_even_) {
      vrt_node_ = this_node_ / 2;
    } else {
      vrt_node_ = -1;
    }
  } else {
    vrt_node_ = this_node_ - nprocs_rem_;
  }
}

Rabenseifner::~Rabenseifner() {
  if (collection_proxy_ != u64empty) {
    StateHolder::clearAll(detail::StrongVrtProxy{collection_proxy_});
  } else if (objgroup_proxy_ != u64empty) {
    StateHolder::clearAll(detail::StrongObjGroup{objgroup_proxy_});
  } else {
    StateHolder::clearAll(detail::StrongGroup{group_});
  }
}

} // namespace vt::collective::reduce::allreduce
