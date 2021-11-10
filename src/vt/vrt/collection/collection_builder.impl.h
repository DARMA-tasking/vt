/*
//@HEADER
// *****************************************************************************
//
//                          collection_builder.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_COLLECTION_BUILDER_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_COLLECTION_BUILDER_IMPL_H

#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/param/construct_params.h"
#include "vt/topos/mapping/dense/unbounded_default.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT>
std::tuple<EpochType, VirtualProxyType> CollectionManager::makeCollection(
  param::ConstructParams<ColT>& po
) {
  auto const is_migratable = po.migratable_;
  auto const is_collective = po.collective_;

  // Generate a new proxy for this new collection
  auto const proxy_bits = makeCollectionProxy(is_collective, is_migratable);
  po.proxy_bits_ = proxy_bits;

  if (not is_collective) {
    auto ep = theTerm()->makeEpochRooted("collection construction");
    theMsg()->pushEpoch(ep);
    using MsgType = param::ConstructParamMsg<ColT>;
    auto m = makeMessage<MsgType>(po);
    theMsg()->broadcastMsg<MsgType, makeCollectionHandler>(m);
    theMsg()->popEpoch(ep);
    theTerm()->finishedEpoch(ep);
    return std::make_tuple(ep, proxy_bits);
  } else {
    auto ep = theTerm()->makeEpochCollective("collection construction");
    theMsg()->pushEpoch(ep);
    makeCollectionImpl(po);
    theMsg()->popEpoch(ep);
    theTerm()->finishedEpoch(ep);
    return std::make_tuple(ep, proxy_bits);
  }
}

template <typename ColT>
/*static*/ void CollectionManager::makeCollectionHandler(
  param::ConstructParamMsg<ColT>* msg
) {
  theCollection()->makeCollectionImpl(*msg->po);
}

namespace detail {
template <typename ColT>
struct ContainableElementFn {
  using IndexType = typename ColT::IndexType;
  explicit ContainableElementFn(std::unique_ptr<ColT> in_elm)
    : elm(std::move(in_elm))
  { }
  std::unique_ptr<ColT> operator()(IndexType) { return std::move(elm); }
  std::unique_ptr<ColT> elm;
};
} /* end namespace detail */

template <typename ColT>
void CollectionManager::makeCollectionImpl(param::ConstructParams<ColT>& po) {
  using IndexType = typename ColT::IndexType;

  if (not po.has_bounds_ and po.bulk_inserts_.size() == 1) {
    po.bounds_ = po.bulk_inserts_[0];
    po.has_bounds_ = true;
  }

  auto const proxy = po.proxy_bits_;
  auto const has_dynamic_membership = po.dynamic_membership_;
  auto const this_node = theContext()->getNode();
  auto const has_bounds = po.has_bounds_;
  auto const bounds = has_bounds ? po.bounds_ : IndexType{};

  // Setup a proper default map if none is explicitly specified by the user
  if (po.map_han_ == uninitialized_handler and po.map_object_ == no_obj_group) {
    if (has_bounds) {
      po.map_han_ = CollectionManager::getDefaultMap<ColT>();
    } else {
      po.map_object_ = mapping::UnboundedDefaultMap<IndexType>::construct();
    }
  }

  auto const map_han = po.map_han_;
  auto const map_object = po.map_object_;

  // Invoke getCollectionLM() to create a new location manager instance for
  // this collection
  theLocMan()->getCollectionLM<ColT, IndexType>(proxy);

  // Insert action on cleanup for this collection
  addCleanupFn<ColT>(proxy);

  // Insert some typeless basic meta-data about the collection
  insertCollectionInfo(proxy, map_han);

  // Insert the typed meta-data for this new collection, along with creating
  // the meta-data collection holder for elements
  insertMetaCollection<ColT>(
    proxy, map_han, has_dynamic_membership, map_object, has_bounds, bounds
  );

  std::size_t global_constructed_elms = 0;

  if (po.bulk_insert_bounds_) {
    vtAssert(
      po.bulk_inserts_.size() == 0,
      "To bulk insert bounds, the collection must have no other bulk insertions"
    );
    vtAssert(
      po.list_inserts_.size() == 0,
      "To bulk insert bounds, the collection must have no other list insertions"
    );
  }

  if (po.bulk_insert_bounds_ and po.has_bounds_) {
    po.bulk_inserts_.push_back(po.bounds_);
  }

  auto cons_fn = po.template getConsFn<ColT>();

  // Do all bulk insertions
  for (auto&& range : po.bulk_inserts_) {
    range.foreach([&](IndexType idx) {
      if (elementMappedHere(map_han, map_object, idx, bounds)) {
        makeCollectionElement<ColT>(proxy, idx, this_node, cons_fn);
      }
      global_constructed_elms++;
    });
  }

  // Do all list insertions
  for (auto&& list_fn : po.list_inserts_) {
    list_fn([&](IndexType idx) {
      if (elementMappedHere(map_han, map_object, idx, bounds)) {
        makeCollectionElement<ColT>(proxy, idx, this_node, cons_fn);
      }
      global_constructed_elms++;
    });
  }

  // Do all 'here' insertions
  for (auto&& elm : po.list_insert_here_) {
    auto const idx = std::get<0>(elm);
    detail::ContainableElementFn<ColT> c{std::move(std::get<1>(elm))};
    makeCollectionElement<ColT>(proxy, idx, this_node, std::move(c));
  }

  if (global_constructed_elms != 0) {
    // Construct a underlying group for the collection
    constructGroup<ColT>(proxy);
  }
}

template <typename ColT, typename Callable>
void CollectionManager::makeCollectionElement(
  VirtualProxyType const proxy, typename ColT::IndexType idx,
  NodeType const mapped_node, Callable&& cons_fn, bool zero_reduce_stamp
) {
  using IndexType        = typename ColT::IndexType;
  using IdxContextHolder = CollectionContextHolder<IndexType>;

  auto prev_index = IdxContextHolder::index();
  auto prev_proxy = IdxContextHolder::proxy();

  // Set the current context index to `idx`. This enables the user to
  // query the index of their collection element in the constructor
  IdxContextHolder::set(&idx, proxy);

  // Invoke the construct function for the collection element
  auto elm = cons_fn(idx);

  // Through the attorney, setup all the properties on the newly constructed
  // collection element: index and proxy
  CollectionTypeAttorney::setup(elm.get(), idx, proxy);

  // If true, set the reduce stamp to zero so we can correctly identify new
  // collection elements during an insertion epoch to compute the min stamp
  // after insertions complete
  if (zero_reduce_stamp) {
    elm->zeroReduceStamp();
  }

  // Insert the element into the managed holder for elements
  insertCollectionElement<ColT>(std::move(elm), proxy, idx, mapped_node);

  // Clear the current index context
  IdxContextHolder::clear();

  // Restore back the previous context for recursive constructions
  IdxContextHolder::set(prev_index, prev_proxy);
}

template <typename IdxT>
bool CollectionManager::elementMappedHere(
  HandlerType map_han, ObjGroupProxyType map_object, IdxT idx, IdxT bounds
) {
  auto const this_node = theContext()->getNode();
  auto const mapped_node = getElementMapping(map_han, map_object, idx, bounds);
  return mapped_node == this_node;
}

template <typename IdxT>
NodeType CollectionManager::getElementMapping(
  HandlerType map_han, ObjGroupProxyType map_object, IdxT idx, IdxT bounds
) {
  if (map_han != uninitialized_handler) {
    // Get the map handler function
    bool const is_functor =
      auto_registry::HandlerManagerType::isHandlerFunctor(map_han);
    auto_registry::AutoActiveMapType map_fn = nullptr;
    if (is_functor) {
      map_fn = auto_registry::getAutoHandlerFunctorMap(map_han);
    } else {
      map_fn = auto_registry::getAutoHandlerMap(map_han);
    }

    auto const num_nodes = theContext()->getNumNodes();
    auto const mapped_node = map_fn->dispatch(&idx, &bounds, num_nodes);
    return mapped_node;
  }

  if (map_object != no_obj_group) {
    objgroup::proxy::Proxy<mapping::BaseMapper<IdxT>> p{map_object};
    auto map_obj_ptr = p.get();
    auto num_nodes = theContext()->getNumNodes();
    return map_obj_ptr->map(&idx, idx.ndims(), num_nodes);
  }

  vtAbort("No valid map fn or object group specified for the collection");
  return uninitialized_destination;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_COLLECTION_BUILDER_IMPL_H*/
