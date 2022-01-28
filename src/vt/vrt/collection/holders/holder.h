/*
//@HEADER
// *****************************************************************************
//
//                                   holder.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_HOLDERS_HOLDER_H
#define INCLUDED_VT_VRT_COLLECTION_HOLDERS_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/holders/elm_holder.h"
#include "vt/vrt/collection/types/headers.h"
#include "vt/vrt/collection/messages/user.h"
#include "vt/vrt/collection/listener/listen_events.h"

#include <unordered_map>
#include <tuple>
#include <list>
#include <memory>
#include <functional>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection {

/**
 * \struct Holder
 *
 * \brief Hold collection elements mapped to this node for a given collection
 *
 * Store the unique pointers to collection elements for a given collection
 * proxy. Provides functionality to find, add, remove, and foreach over the
 * collection elements.
 */
template <typename IndexT>
struct Holder {
  template <typename T, typename U>
  using ContType            = std::unordered_map<T, U>;
  using CollectionType      = Indexable<IndexT>;
  using VirtualPtrType      = std::unique_ptr<CollectionType>;
  using LookupElementType   = IndexT;
  using InnerHolder         = ElementHolder<IndexT>;
  using TypedIndexContainer = ContType<LookupElementType, InnerHolder>;
  using LBContFnType        = std::function<void()>;
  using LBContListType      = std::list<LBContFnType>;
  using TypedLBContainer    = ContType<LookupElementType, LBContListType>;
  using FuncApplyType       = std::function<void(IndexT const&, CollectionType*)>;
  using FuncExprType        = std::function<bool(IndexT const&)>;
  using CountType           = uint64_t;

  /**
   * \brief Check of index exists here
   *
   * \param[in] idx the index
   *
   * \return whether it exists
   */
  bool exists(IndexT const& idx);

  /**
   * \brief Lookup the inner holder for an index
   *
   * \param[in] idx the index
   *
   * \return the inner holder with the actual collection element
   */
  InnerHolder& lookup(IndexT const& idx);

  /**
   * \brief Insert a new element
   *
   * \param[in] idx the index
   * \param[in] inner the inner holder with the collection element unique pointer
   */
  void insert(IndexT const& idx, InnerHolder&& inner);

  /**
   * \brief Remove an element
   *
   * \param[in] idx the index
   *
   * \return unique pointer to element now removed from holder
   */
  VirtualPtrType remove(IndexT const& idx);

  /**
   * \brief Destroy all elements
   */
  void destroyAll();

  /**
   * \brief Check if collection has been destroyed
   *
   * \return whether it is destroyed
   */
  bool isDestroyed() const;

  /**
   * \brief Cleanup deleted elements that were delayed
   *
   * \todo Rename this method
   *
   * When deleting elements from a handler, it might not be safe to delete them
   * from the holder right away due to a reference/iterator to the element being
   * held while deletion occurs. This method cleans up an elements that are
   * marked as erased.
   */
  void cleanupExists();

  /**
   * \brief Perform apply action over all collection elements
   *
   * \param[in] fn apply function for each element
   */
  void foreach(FuncApplyType fn);

  /**
   * \brief Count number of elements
   *
   * \return number of elements
   */
  typename TypedIndexContainer::size_type numElements() const;

  /**
   * \brief Count number of elements that match a expression
   *
   * \param[in] f apply function that returns membership of expression
   *
   * \return number of elements
   */
  typename TypedIndexContainer::size_type numElementsExpr(FuncExprType f) const;

  /**
   * \brief Get current group
   *
   * \return the group ID
   */
  GroupType group() const { return cur_group_; }

  /**
   * \brief Set a group
   *
   * \param[in] group group to set
   */
  void setGroup(GroupType const& group) { cur_group_ = group; }

  /**
   * \brief Whether the group should be used
   *
   * \return whether it should be used
   */
  bool useGroup() const { return use_group_; }

  /**
   * \brief Set whether a group should be used
   *
   * \param[in] use_group whether it should be used
   */
  void setUseGroup(bool const use_group) { use_group_ = use_group; }

  /**
   * \brief Check if group is ready
   *
   * \return whether it is ready
   */
  bool groupReady() const { return group_ready_; }

  /**
   * \brief Set if group is ready to be used
   *
   * \param[in] ready whether it is ready
   */
  void setGroupReady(bool const ready) { group_ready_ = ready; }

  /**
   * \brief Get the root of the group
   *
   * \return the group root
   */
  NodeType groupRoot() const { return group_root_; }

  /**
   * \brief Set the root of the group
   *
   * \param[in] root the root
   */
  void setGroupRoot(NodeType const root) { group_root_ = root; }

  /**
   * \brief Add element-specific listener
   *
   * \param[in] fn listener function
   *
   * \return the registered listener entry ID
   */
  int addListener(listener::ListenFnType<IndexT> fn);

  /**
   * \brief Remove a element listener
   *
   * \param[in] element the entry ID
   */
  void removeListener(int element);

  /**
   * \brief Run all listeners
   *
   * \param[in] event the event type
   * \param[in] idx the index to run on
   * \param[in] home the home node for the element
   */
  void applyListeners(
    listener::ElementEventEnum event, IndexT const& idx, NodeType home_node
  );

  friend struct CollectionManager;

private:
  bool erased                                                     = false;
  typename TypedIndexContainer::iterator foreach_iter             = {};
  TypedIndexContainer vc_container_                               = {};
  bool is_destroyed_                                              = false;
  GroupType cur_group_                                            = no_group;
  bool use_group_                                                 = false;
  bool group_ready_                                               = false;
  NodeType group_root_                                            = 0;
  CountType num_erased_not_removed_                               = 0;
  std::vector<listener::ListenFnType<IndexT>> event_listeners_    = {};
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/holder.impl.h"

#endif /*INCLUDED_VT_VRT_COLLECTION_HOLDERS_HOLDER_H*/
