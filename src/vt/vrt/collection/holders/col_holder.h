/*
//@HEADER
// *****************************************************************************
//
//                                 col_holder.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_HOLDERS_COL_HOLDER_H
#define INCLUDED_VT_VRT_COLLECTION_HOLDERS_COL_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/collection/holders/holder.h"
#include "vt/vrt/collection/holders/base_holder.h"

namespace vt { namespace vrt { namespace collection {

/**
 * \struct CollectionHolder
 *
 * \brief Holds meta-data for the entire collection
 *
 * Manages the meta-data for a collection including the registered map function,
 * index range, and holder for the actual elements mapped to this node. Provides
 * the virtual overloaded function for running LB and destroying the collection
 * from a type-erased base class.
 */
template <typename IndexT>
struct CollectionHolder : BaseHolder {

public:
  /**
   * \internal \brief Construct a collection holder for meta-data
   *
   * \param[in] in_map_fn the map function
   * \param[in] in_has_dynamic_membership collection has dynamic membership?
   * \param[in] in_map_object the map object
   * \param[in] in_has_bounds whether it has bounds
   * \param[in] in_bounds the bounds
   */
  CollectionHolder(
    HandlerType const in_map_fn, bool const in_has_dynamic_membership,
    ObjGroupProxyType in_map_object, bool const in_has_bounds,
    IndexT const in_bounds
  );

  virtual ~CollectionHolder() {}

public:
  /**
   * \internal \brief Destroy the collection
   */
  void destroy() override;

  HandlerType map_fn = uninitialized_handler;  /**< The map function */
  bool has_dynamic_membership_ = false;        /**< Whether has dynamic membership */
  ObjGroupProxyType map_object = no_obj_group; /**< The map object */
  bool has_bounds = false;                     /**< Whether it as bounds */
  IndexT bounds = {};                          /**< The bounds */
  Holder<IndexT> holder_;                      /**< Inner holder of elements */
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/col_holder.impl.h"

#endif /*INCLUDED_VT_VRT_COLLECTION_HOLDERS_COL_HOLDER_H*/
