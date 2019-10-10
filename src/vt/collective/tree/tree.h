/*
//@HEADER
// *****************************************************************************
//
//                                    tree.h
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

#if !defined INCLUDED_COLLECTIVE_TREE_TREE_H
#define INCLUDED_COLLECTIVE_TREE_TREE_H

#include "vt/config.h"

#include <vector>
#include <functional>
#include <cstdlib>

namespace vt { namespace collective { namespace tree {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma sst keep
static struct DefaultTreeConstructTag { } tree_cons_tag_t { };
#pragma GCC diagnostic pop

struct Tree {
  using NodeListType = std::vector<NodeType>;
  using OperationType = std::function<void(NodeType)>;
  using NumLevelsType = int32_t;

  explicit Tree(DefaultTreeConstructTag);
  explicit Tree(NodeListType const& in_children);

  Tree(
    bool const in_is_root, NodeType const& parent,
    NodeListType const& in_children
  );

  void setupTree();
  NodeType getParent() const;
  NodeType getNumChildren() const;
  bool isRoot() const;
  NodeListType const& getChildren() const;
  NodeListType getChildren(NodeType node) const;
  void foreachChild(OperationType op) const;
  NumLevelsType numLevels() const;
  void foreachChild(NumLevelsType level, OperationType op) const;
  std::size_t getNumTotalChildren(NodeType child) const;
  std::size_t getNumTotalChildren() const;

private:
  bool set_up_tree_ = false;
  NodeType parent_ = uninitialized_destination;
  bool is_root_ = false;
  NodeListType children_;
};

}}} //end namespace vt::collective::tree

#endif /*INCLUDED_COLLECTIVE_TREE_TREE_H*/
