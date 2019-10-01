/*
//@HEADER
// *****************************************************************************
//
//                                   holder.h
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

#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/holders/elm_holder.h"
#include "vt/vrt/collection/types/headers.h"
#include "vt/vrt/collection/messages/user.h"

#include <unordered_map>
#include <tuple>
#include <list>
#include <memory>
#include <functional>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Holder {
  template <typename T, typename U>
  using ContType            = std::unordered_map<T, U>;
  using CollectionType      = CollectionBase<ColT, IndexT>;
  using VirtualPtrType      = std::unique_ptr<CollectionType>;
  using LookupElementType   = IndexT;
  using InnerHolder         = ElementHolder<ColT, IndexT>;
  using TypedIndexContainer = ContType<LookupElementType, InnerHolder>;
  using LBContFnType        = std::function<void()>;
  using LBContListType      = std::list<LBContFnType>;
  using TypedLBContainer    = ContType<LookupElementType, LBContListType>;
  using FuncApplyType       = std::function<void(IndexT const&, CollectionType*)>;
  using FuncExprType        = std::function<bool(IndexT const&)>;
  using CountType           = uint64_t;

  bool exists(IndexT const& idx);
  InnerHolder& lookup(IndexT const& idx);
  void insert(IndexT const& idx, InnerHolder&& inner);
  VirtualPtrType remove(IndexT const& idx);
  void destroyAll();
  bool isDestroyed() const;
  void cleanupExists();
  bool foreach(FuncApplyType fn);
  typename TypedIndexContainer::size_type numElements() const;
  typename TypedIndexContainer::size_type numElementsExpr(FuncExprType f) const;
  GroupType group() const { return cur_group_; }
  void setGroup(GroupType const& group) { cur_group_ = group; }
  bool useGroup() const { return use_group_; }
  void setUseGroup(bool const use_group) { use_group_ = use_group; }
  bool groupReady() const { return group_ready_; }
  void setGroupReady(bool const ready) { group_ready_ = ready; }
  NodeType groupRoot() const { return group_root_; }
  void setGroupRoot(NodeType const root) { group_root_ = root; }
  CountType numReady() const { return elements_ready_; }
  void addReady(CountType num = 1) { elements_ready_ += 1; }
  void clearReady() { elements_ready_ = 0; }
  void addLBCont(IndexT const& idx, LBContFnType fn);
  void runLBCont(IndexT const& idx);
  void runLBCont();

  friend struct CollectionManager;

private:
  bool erased                                                     = false;
  typename TypedIndexContainer::iterator foreach_iter             = {};
  std::unordered_map<EpochType, CollectionMessage<ColT>*> bcasts_ = {};
  EpochType cur_bcast_epoch_                                      = 1;
  TypedIndexContainer vc_container_                               = {};
  TypedLBContainer vc_lb_continuation_                            = {};
  bool is_destroyed_                                              = false;
  GroupType cur_group_                                            = no_group;
  bool use_group_                                                 = false;
  bool group_ready_                                               = false;
  NodeType group_root_                                            = 0;
  CountType num_erased_not_removed_                               = 0;
  CountType elements_ready_                                       = 0;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H*/
