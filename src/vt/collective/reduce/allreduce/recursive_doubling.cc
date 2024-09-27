/*
//@HEADER
// *****************************************************************************
//
//                            recursive_doubling.cc
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

#include "vt/collective/reduce/allreduce/recursive_doubling.h"
#include "vt/collective/reduce/allreduce/helpers.h"
#include "vt/group/group_manager.h"

namespace vt::collective::reduce::allreduce {

RecursiveDoubling::RecursiveDoubling(
  detail::StrongVrtProxy proxy, detail::StrongGroup group, size_t num_elems)
  : info_({ComponentT::VrtColl, proxy.get()}),
    local_num_elems_(num_elems),
    nodes_(theGroup()->GetGroupNodes(group.get())),
    num_nodes_(nodes_.size()),
    this_node_(theContext()->getNode()),
    num_steps_(static_cast<uint32_t>(std::log2(num_nodes_))),
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
  initializeVrtNode();

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling (this={}): proxy={:x} local_num_elems={}\n",
    print_ptr(this), proxy.get(), local_num_elems_);
}

RecursiveDoubling::RecursiveDoubling(detail::StrongObjGroup objgroup)
  : info_({ComponentT::ObjGroup, objgroup.get()}),
    num_nodes_(theContext()->getNumNodes()),
    this_node_(vt::theContext()->getNode()),
    is_even_(this_node_ % 2 == 0),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_),
    is_part_of_adjustment_group_(this_node_ < (2 * nprocs_rem_)) {
  nodes_.resize(num_nodes_);
  for (NodeType i = 0; i < theContext()->getNumNodes(); ++i) {
    nodes_[i] = i;
  }

  initializeVrtNode();
}

RecursiveDoubling::RecursiveDoubling(detail::StrongGroup group)
  : info_({ComponentT::Group, group.get()}),
    nodes_(theGroup()->GetGroupNodes(group.get())),
    num_nodes_(nodes_.size()),
    this_node_(vt::theContext()->getNode()),
    is_even_(this_node_ % 2 == 0),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_),
    is_part_of_adjustment_group_(this_node_ < (2 * nprocs_rem_)) {
  initializeVrtNode();
}

void RecursiveDoubling::initializeVrtNode() {
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

RecursiveDoubling::~RecursiveDoubling() {
if (info_.first == ComponentT::ObjGroup) {
    StateHolder::clearAll(detail::StrongObjGroup{info_.second});
    AllreduceHolder::remove(detail::StrongObjGroup{info_.second});
  } else if(info_.first == ComponentT::Group){
    StateHolder::clearAll(detail::StrongGroup{info_.second});
  }
}

} // namespace vt::collective::reduce::allreduce
