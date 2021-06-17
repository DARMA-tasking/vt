/*
//@HEADER
// *****************************************************************************
//
//                                   tree.cc
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
#include "vt/collective/tree/tree.h"
#include "vt/context/context.h"

#include <cmath>
#include <cstdlib>

namespace vt { namespace collective { namespace tree {

Tree::Tree(DefaultTreeConstructTag) {
  setupTree();
}

Tree::Tree(
  bool const in_is_root, NodeType const& parent, NodeListType const& in_children
) {
  is_root_ = in_is_root;
  children_ = in_children;
  parent_ = parent;
  set_up_tree_ = true;
}

Tree::Tree(NodeListType const& in_children) {
  is_root_ = true;
  children_ = in_children;
}

NodeType Tree::getParent() const {
  return parent_;
}

NodeType Tree::getNumChildren() const {
  return children_.size();
}

bool Tree::isRoot() const {
  return is_root_;
}

Tree::NodeListType Tree::getChildren(NodeType node) const {
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& c1_ = node * 2 + 1;
  auto const& c2_ = node * 2 + 2;
  NodeListType children;
  if (c1_ < num_nodes) {
    children.push_back(c1_);
  }
  if (c2_ < num_nodes) {
    children.push_back(c2_);
  }
  return children;
}

std::size_t Tree::getNumDescendants(NodeType child) const {
  std::size_t total = 0;
  auto children = getChildren(child);
  for (auto&& elm : children) {
    total += getNumDescendants(elm);
  }
  return total + children.size();
}

std::size_t Tree::getNumDescendants() const {
  auto total_size = 0;
  foreachChild([this,&total_size](NodeType child){
    total_size += getNumDescendants(child);
  });
  return total_size;
}

Tree::NodeListType const& Tree::getChildren() const {
  return children_;
}

void Tree::foreachChild(OperationType op) const {
  if (children_.size() > 0) {
    for (auto&& elm : children_) {
      op(elm);
    }
  }
}

void Tree::foreachChild(NumLevelsType level, OperationType op) const {
  int32_t const start = std::pow(2.0f, level) - 1;
  int32_t const end = std::pow(2.0f, level + 1) - 1;
  for (auto i = start; i < end; i++) {
    op(i);
  }
}

void Tree::setupTree() {
  if (not set_up_tree_) {
    auto const& this_node_ = theContext()->getNode();
    auto const& num_nodes_ = theContext()->getNumNodes();

    auto const& c1_ = this_node_ * 2 + 1;
    auto const& c2_ = this_node_ * 2 + 2;

    if (c1_ < num_nodes_) {
      children_.push_back(c1_);
    }
    if (c2_ < num_nodes_) {
      children_.push_back(c2_);
    }

    is_root_ = this_node_ == 0;

    if (not is_root_) {
      parent_ = (this_node_ - 1) / 2;
    }

    set_up_tree_ = true;
  }
}

Tree::NumLevelsType Tree::numLevels() const {
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& levels = std::log2(num_nodes);
  return levels;
}

}}} //end namespace vt::collective::tree
