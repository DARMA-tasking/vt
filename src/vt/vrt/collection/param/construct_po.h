/*
//@HEADER
// *****************************************************************************
//
//                                construct_po.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PO_H
#define INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PO_H

#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/registry/auto/map/auto_registry_map.h"

#include <functional>
#include <memory>

#include <checkpoint/checkpoint.h>

namespace vt { namespace vrt { namespace collection {

struct CollectionManager;

}}} /* end namespace vt::vrt::collection */

namespace vt { namespace vrt { namespace collection { namespace param {

/// fwd-declare the PO constructor
template <typename ColT>
struct ConstructParams;

/// fwd-declare the builder helper function
template <typename ColT>
ConstructParams<ColT> makeCollectionImpl(bool const is_collective);

/**
 * \struct ConstructParams
 *
 * \brief A parameter configuration object for building a collection
 */
template <typename ColT>
struct ConstructParams {
  using ThisType          = ConstructParams<ColT>;
  using IndexType         = typename ColT::IndexType;
  using ProxyType         = CollectionProxy<ColT>;
  using ApplyFnType       = std::function<void(IndexType)>;
  using ProxyFnType       = std::function<void(ProxyType)>;
  using VirtualPtrType    = std::unique_ptr<CollectionBase<ColT,IndexType>>;
  using ConstructFnType   = std::function<VirtualPtrType(IndexType)>;
  using ListInsertType    = std::function<void(ApplyFnType)>;
  using ListInsertElmType = std::tuple<IndexType, std::unique_ptr<ColT>>;

private:
  struct BuilderTag{};

  ConstructParams(BuilderTag, bool const in_is_collective)
    : collective_(in_is_collective)
  {}

  friend ThisType makeCollectionImpl<ColT>(bool const);

public:
  ConstructParams() = default;
  ConstructParams(ConstructParams&&) = default;

  ConstructParams(ConstructParams const& x)
    : bounds_(x.bounds_),
      has_bounds_(x.has_bounds_),
      bulk_inserts_(x.bulk_inserts_),
      bulk_insert_bounds_(x.bulk_insert_bounds_),
      cons_fn_(x.cons_fn_),
      dynamic_membership_(x.dynamic_membership_),
      collective_(x.collective_),
      constructed_(x.constructed_),
      migratable_(x.migratable_),
      map_han_(x.map_han_),
      proxy_bits_(x.proxy_bits_),
      map_object_(x.map_object_)
  {
    vtAssert(
      not collective_,
      "Should only call copy ctor during rooted construction"
    );
    if (x.list_inserts_.size() > 0 or
        x.list_insert_here_.size() > 0) {
      vtAbort(
        "listInsert/listInsertHere can not be used with rooted construction"
      );
    }
    if (x.cons_fn_) {
      vtAbort("elementConstructor can not be used with rooted construction");
    }
  }

  friend CollectionManager;

  ~ConstructParams() {
    vtAssert(
      constructed_,
      "Called makeCollection() without finishing construction by calling"
      " wait() or deferWithEpoch()"
    );
  }

  /**
   * \brief Specify the bounds for the collection. If it doesn't have dynamic
   * membership this whole range will be constructed.
   *
   * \param[in] in_bounds the collection bounds
   */
  ThisType&& bounds(IndexType in_bounds) {
    bounds_ = in_bounds;
    has_bounds_ = true;
    return std::move(*this);
  }

  /**
   * \brief Whether this is collection is migratable
   *
   * \param[in] is_migratable is migratable?
   */
  ThisType&& migratable(bool is_migratable) {
    vtAssert(false, "Currently this is not implemented");
    migratable_ = is_migratable;
    return std::move(*this);
  }

  /**
   * \brief Explicitly specify and register a map for the collection
   */
  template <mapping::ActiveMapTypedFnType<IndexType> fn>
  ThisType&& mapperFunc() {
    map_han_ = auto_registry::makeAutoHandlerMap<IndexType, fn>();
    return std::move(*this);
  }

  /**
   * \brief Explicitly specify and register a map for the collection
   */
  ThisType&& mapperHandler(HandlerType in_map_han) {
    map_han_ = in_map_han;
    return std::move(*this);
  }

  /**
   * \brief Explicitly specify an existing objgroup for the mapper
   */
  template <typename T, typename ProxyT>
  ThisType&& mapperObjGroup(ProxyT proxy) {
    map_object_ = proxy.getProxy();
    return std::move(*this);
  }

  /**
   * \brief Explicitly specify an existing objgroup for the mapper
   *
   * \warning Only valid as a collective invocation
   */
  template <typename T, typename... Args>
  ThisType&& mapperObjGroup(Args&&... args) {
    auto proxy = T::construct(std::forward<Args>(args)...);
    map_object_ = proxy.getProxy();
    return std::move(*this);
  }

  /**
   * \brief Specify a non-default constructor for each element
   *
   * \note Only valid with collective construction
   *
   * \param[in] in_cons_fn the construction function
   */
  ThisType&& elementConstructor(ConstructFnType in_cons_fn) {
    cons_fn_ = in_cons_fn;
    return std::move(*this);
  }

  /**
   * \brief Specify whether the collection has dynamic membership
   *
   * \param[in] is_dynamic_membership dynamic membership?
   */
  ThisType&& dynamicMembership(bool is_dynamic_membership) {
    dynamic_membership_ = is_dynamic_membership;
    return std::move(*this);
  }

  /**
   * \brief Bulk insert a range for the collection
   *
   * \param[in] in_bulk_range the bulk insertion range
   */
  ThisType&& bulkInsert(IndexType in_bulk_range) {
    bulk_inserts_.emplace_back(std::move(in_bulk_range));
    return std::move(*this);
  }

  /**
   * \brief Bulk insert the entire bounds of a collection
   */
  ThisType&& bulkInsert() {
    bulk_insert_bounds_ = true;
    return std::move(*this);
  }

  /**
   * \brief Insert a list of elements
   *
   * \note Only valid with collective construction
   *
   * \param[in] begin begin iterator
   * \param[in] end end iterator
   */
  template <typename Iter>
  ThisType&& listInsert(Iter begin, Iter end) {
    list_inserts_.emplace_back([=](ApplyFnType apply){
      for (auto it = begin; it != end; ++it) {
        apply(*it);
      }
    });
    return std::move(*this);
  }

  /**
   * \brief Insert a list of elements
   *
   * \note Only valid with collective construction
   *
   * \param[in] c the iterable container of elements to insert
   */
  template <typename Container>
  ThisType&& listInsert(Container const& c) {
    return listInsert(c.begin(), c.end());
  }

  /**
   * \brief Insert a list of elements here (on this node)
   *
   * \note Only valid with collective construction
   *
   * \param[in] c the iterable container of elements to insert
   */
  template <typename Container>
  ThisType&& listInsertHere(Container&& c) {
    return listInsertHere(c.begin(), c.end());
  }

  /**
   * \brief Insert a list of elements here (on this node)
   *
   * \note Only valid with collective construction
   *
   * \param[in] c the iterable container of elements to insert
   */
  template <typename Iter>
  ThisType&& listInsertHere(Iter begin, Iter end) {
    for (auto&& iter = begin; iter != end; ++iter) {
      list_insert_here_.emplace_back(std::move(*iter));
    }
    return std::move(*this);
  }

  /**
   * \brief Construct and return an epoch to wait on for construction to finish
   *
   * \param[in] cb callback with the proxy once the collection's construction is
   * complete
   *
   * \return the epoch to wait on
   */
  EpochType deferWithEpoch(ProxyFnType cb);

  /**
   * \brief Wait for construction to complete and then return the proxy
   *
   * \return the proxy after the construction is complete
   */
  ProxyType wait();

  /**
   * \internal \brief Specify whether this is a collective construction of the
   * collection
   *
   * \warning Only for use internally for testing
   *
   * \param[in] is_collective is collective?
   */
  ThisType&& collective(bool is_collective) {
    collective_ = is_collective;
    return std::move(*this);
  }

private:
  /**
   * \brief Get the element constructor function
   */
  template <typename U>
  ConstructFnType getConsFn(
    typename std::enable_if_t<std::is_default_constructible<U>::value>* = nullptr
  ) const {
    if (cons_fn_) {
      return cons_fn_;
    } else {
      return [](IndexType){return std::make_unique<ColT>();};
    }
  }

  /**
   * \brief Get the element constructor function
   */
  template <typename U>
  ConstructFnType getConsFn(
    typename std::enable_if_t<!std::is_default_constructible<U>::value>* = nullptr
  ) const {
    vtAssert(cons_fn_, "Must have constructor fn if not default constructible");
    return cons_fn_;
  }

  /**
   * \brief Validate the configuration inputs before constructing the collection
   */
  void validateInputs() {
    if (list_inserts_.size() > 0) {
      vtAssert(collective_, "Must be a collective construct to insert a list");
    }
    if (not dynamic_membership_) {
      vtAssert(has_bounds_, "Must have valid bounds");
    }
  }

public:
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | bounds_
      | has_bounds_
      | bulk_inserts_
      | bulk_insert_bounds_
      | dynamic_membership_
      | collective_
      | constructed_
      | migratable_
      | map_han_
      | proxy_bits_
      | map_object_;
    s.skip(list_inserts_);
    s.skip(list_insert_here_);
    s.skip(cons_fn_);
  }

private:
  IndexType bounds_                                = {};
  bool has_bounds_                                 = false;
  std::vector<IndexType> bulk_inserts_             = {};
  std::vector<ListInsertType> list_inserts_        = {};
  std::vector<ListInsertElmType> list_insert_here_ = {};
  bool bulk_insert_bounds_                         = false;
  ConstructFnType cons_fn_                         = nullptr;
  bool dynamic_membership_                         = false;
  bool collective_                                 = true;
  bool constructed_                                = false;
  bool migratable_                                 = true;
  HandlerType map_han_                             = uninitialized_handler;
  VirtualProxyType proxy_bits_                     = no_vrt_proxy;
  ObjGroupProxyType map_object_                    = no_obj_group;
};

template <typename ColT>
ConstructParams<ColT> makeCollectionImpl(bool const is_collective) {
  using ConsType = ConstructParams<ColT>;
  using TagType  = typename ConsType::BuilderTag;
  return ConsType{TagType{}, is_collective};
}

}}}} /* end namespace vt::vrt::collection::param */

namespace vt {

/**
 * \brief Construct a new collective collection with the parameter object
 * builder
 *
 * \param[in] bounds the bounds for the collection (optional)
 *
 * \return the parameter configuration object
 */
template <typename ColT>
vrt::collection::param::ConstructParams<ColT> makeCollection() {
  bool const is_collective = true;
  return vrt::collection::param::makeCollectionImpl<ColT>(is_collective);
}

/**
 * \brief Construct a new rooted collection with the parameter object builder
 *
 * \param[in] bounds the bounds for the collection (optional)
 *
 * \return the parameter configuration object
 */
template <typename ColT>
vrt::collection::param::ConstructParams<ColT> makeCollectionRooted() {
  bool const is_collective = false;
  return vrt::collection::param::makeCollectionImpl<ColT>(is_collective);
}

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PO_H*/
