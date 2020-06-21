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

/**
 * \internal \struct Tree
 *
 * \brief General interface for storing a spanning tree
 *
 * Holds the portion of a spanning tree on each node. Does not do any parallel
 * coordination. Higher-level components must keep track of the pieces of the
 * tree on each node.
 */
struct Tree {
  using NodeListType = std::vector<NodeType>;
  using OperationType = std::function<void(NodeType)>;
  using NumLevelsType = int32_t;

  /**
   * \internal \brief Construct the default spanning tree across the whole
   * communicator.
   *
   * \param[in] DefaultTreeConstructTag constructor tag
   */
  explicit Tree(DefaultTreeConstructTag);

  /**
   * \internal \brief Construct a spanning tree with a list of children nodes on
   * the root node
   *
   * \warning This should only be called on the root.
   *
   * \param[in] in_children the list of children
   */
  explicit Tree(NodeListType const& in_children);

  /**
   * \internal \brief Construct a spanning tree with a list of children nodes,
   * parent node, and whether this is the root.
   *
   * \param[in] in_is_root if this is the root node
   * \param[in] parent the node's parent in the tree
   * \param[in] in_children the list of children
   */
  Tree(
    bool const in_is_root, NodeType const& parent,
    NodeListType const& in_children
  );

  /**
   * \internal \brief Setup the default (binomial) tree
   */
  void setupTree();

  /**
   * \internal \brief Get the parent node in the tree
   *
   * \return the parent node
   */
  NodeType getParent() const;

  /**
   * \internal \brief Get the number of children nodes
   *
   * \return the number of children nodes
   */
  NodeType getNumChildren() const;

  /**
   * \internal \brief Get whether this node is the root
   *
   * \return whether this node is the root
   */
  bool isRoot() const;

  /**
   * \internal \brief Get the (const) list of children
   *
   * \return (const) list of children
   */
  NodeListType const& getChildren() const;

  /**
   * \internal \brief Get the list of children for a particular node in the tree
   *
   * \return list of children
   */
  NodeListType getChildren(NodeType node) const;

  /**
   * \internal \brief Apply function (foreach) across all children
   *
   * \param[in] op action to apply, passed the node
   */
  void foreachChild(OperationType op) const;

  /**
   * \internal \brief Get number of levels in the tree
   *
   * \return number of levels
   */
  NumLevelsType numLevels() const;

  /**
   * \internal \brief Apply function (foreach) across all children with number
   * of levels passed to apply function.
   *
   * \param[in] level number of levels in spanning tree
   * \param[in] op action to apply, passed levels and node
   */
  void foreachChild(NumLevelsType level, OperationType op) const;

  /**
   * \internal \brief Get total number of descendants for a particular node in
   * the tree
   *
   * \param[in] child the node
   *
   * \return number of descendants
   */
  std::size_t getNumTotalChildren(NodeType child) const;

  /**
   * \internal \brief Get total children in tree
   *
   * \return number of children
   */
  std::size_t getNumTotalChildren() const;

private:
  bool set_up_tree_ = false;
  NodeType parent_ = uninitialized_destination;
  bool is_root_ = false;
  NodeListType children_;
};

}}} //end namespace vt::collective::tree

#endif /*INCLUDED_COLLECTIVE_TREE_TREE_H*/
