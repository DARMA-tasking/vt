/*
//@HEADER
// *****************************************************************************
//
//                                  scatter.cc
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

#include "vt/config.h"
#include "vt/collective/scatter/scatter.h"
#include "vt/collective/collective_alg.h"
#include "vt/messaging/active.h"

namespace vt { namespace collective { namespace scatter {

Scatter::Scatter()
  : tree::Tree(tree::tree_cons_tag_t)
{ }

char* Scatter::applyScatterRecur(
  NodeType node, char* ptr, std::size_t elm_size, FuncSizeType size_fn,
  FuncDataType data_fn
) {
  // pre-order k-ary tree traversal for data layout
  auto children = Tree::getChildren(node);
  char* cur_ptr = ptr;
  vt_debug_print(
    normal, scatter,
    "Scatter::applyScatterRecur: elm_size={}, ptr={}, node={}\n",
    elm_size, print_ptr(ptr), node
  );
  data_fn(node, reinterpret_cast<void*>(cur_ptr));
  cur_ptr += elm_size;
  for (auto&& child : children) {
    vt_debug_print(
      verbose, scatter,
      "Scatter::applyScatterRecur: child={}\n", child
    );
    cur_ptr = applyScatterRecur(child, cur_ptr, elm_size, size_fn, data_fn);
  }
  return cur_ptr;
}

void Scatter::scatterIn(ScatterMsg* msg) {
  auto const& total_children = getNumDescendants();
  auto const& elm_size = msg->elm_bytes_;
  auto const& total_size = msg->total_bytes_;
  auto in_base_ptr = reinterpret_cast<char*>(msg) + sizeof(ScatterMsg);
  auto in_ptr = in_base_ptr + elm_size;
  auto const& user_handler = msg->user_han;
  vt_debug_print(
    normal, scatter,
    "Scatter::scatterIn: handler={}, total_size={}, elm_size={}, offset={}, "
    "parent children={}\n",
    user_handler, total_size, elm_size, in_ptr - in_base_ptr, total_children
  );
  Tree::foreachChild([&](NodeType child) {
    auto const& num_children = getNumDescendants(child) + 1;
    auto const& child_bytes_size = num_children * elm_size;
    auto child_msg = makeMessageSz<ScatterMsg>(
      child_bytes_size, child_bytes_size, elm_size
    );
    vt_debug_print(
      verbose, scatter,
      "Scatter::scatterIn: child={}, num_children={}, child_bytes_size={}\n",
      child, num_children, child_bytes_size
    );
    auto const child_remaining_size = thePool()->remainingSize(
      reinterpret_cast<void*>(child_msg.get())
    );
    child_msg->user_han = user_handler;
    auto ptr = reinterpret_cast<char*>(child_msg.get()) + sizeof(ScatterMsg);
    vt_debug_print(
      verbose, scatter,
      "Scatter::scatterIn: child={}, num_children={}, elm_size={}, "
      "offset={}, child_remaining={}, parent size={}, child_bytes_size={}\n",
      child, num_children, elm_size, in_ptr - in_base_ptr,
      child_remaining_size, total_size, child_bytes_size
    );
    std::memcpy(ptr, in_ptr, child_bytes_size);
    in_ptr += child_bytes_size;
    theMsg()->sendMsgSz<ScatterMsg,scatterHandler>(
      child, child_msg, sizeof(ScatterMsg) + child_bytes_size
    );
  });
  auto active_fn = auto_registry::getAutoHandler(user_handler);
  active_fn(reinterpret_cast<BaseMessage*>(in_base_ptr));
}

/*static*/ void Scatter::scatterHandler(ScatterMsg* msg) {
  return theCollective()->scatterIn(msg);
}

}}} /* end namespace vt::collective::scatter */
