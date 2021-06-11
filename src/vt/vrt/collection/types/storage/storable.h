/*
//@HEADER
// *****************************************************************************
//
//                                  storable.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORABLE_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORABLE_H

#include "vt/config.h"
#include "vt/vrt/collection/types/storage/store_elm.h"

#include <unordered_map>
#include <memory>

namespace vt { namespace vrt { namespace collection { namespace storage {

/**
 * \struct Storable
 *
 * \brief Trait for collection elements to store values inside the collection
 */
struct Storable {

  Storable() = default;

  /**
   * \brief Serializer
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s);

  /**
   * \brief Insert a new key/value pair
   *
   * \param[in] str the key
   * \param[in] u the value
   */
  template <typename U>
  void valInsert(std::string const& str, U&& u);

  /**
   * \brief Get the value from a key
   *
   * \param[in] str the key
   *
   * \return the associated value
   */
  template <typename U>
  U& valGet(std::string const& str);

  /**
   * \brief Get the const value from a key
   *
   * \param[in] str the key
   *
   * \return the associated, const value
   */
  template <typename U>
  U const& valGet(std::string const& str) const;

  /**
   * \brief Check if a key exists
   *
   * \param[in] str the key
   *
   * \return whether it exists
   */
  bool valExists(std::string const& str) const;

  /**
   * \brief Remove a key
   *
   * \param[in] str the key
   */
  void valRemove(std::string const& str);

private:
  /// Map of type-erased, stored values
  std::unordered_map<std::string, std::unique_ptr<StoreElmBase>> map_;
};

}}}} /* end namespace vt::vrt::collection::storage */

#include "vt/vrt/collection/types/storage/storable.impl.h"

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORABLE_H*/
