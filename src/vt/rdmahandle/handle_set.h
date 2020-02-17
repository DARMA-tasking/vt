/*
//@HEADER
// *****************************************************************************
//
//                                 handle_set.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_SET_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_SET_H

#include "vt/config.h"
#include "vt/rdmahandle/handle.h"
#include "vt/topos/index/index.h"

namespace vt { namespace rdma {

/**
 * \struct HandleSet handle_set.h vt/rdmahandle/handle_set.h
 *
 * \brief Holds a static (non-migratable) set of handles that can be indexed
 * built by an objgroup for sub-chunking
 */
template <typename T>
struct HandleSet {
  using IndexType  = vt::Index2D;
  using HandleType = rdma::Handle<T, HandleEnum::StaticSize, IndexType>;
  using LookupType = int32_t;

  HandleSet(HandleSet const&) = default;
  HandleSet(HandleSet&&) = default;
  HandleSet& operator=(HandleSet const&) = default;
  HandleSet& operator=(HandleSet&&) = default;

  friend struct Manager;

  struct BuildSetTagType { };

private:
  /**
   * \internal \brief Construct a new \c HandleSet called by the system
   */
  HandleSet(BuildSetTagType) { }

  /**
   * \internal \brief Add a handle to the set. Used by the system.
   *
   * \param[in] idx index to add
   * \param[in] han handle to add
   */
  void addHandle(IndexType const& idx, HandleType han) {
    vtAssert(set_.find(idx) == set_.end(), "Index already inserted into set");
    set_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(idx),
      std::forward_as_tuple(han)
    );
  }

  /**
   * \internal \brief Called by the system to indicate all insertions into the
   * set are complete and the handles are valid to use
   */
  void finishedInserts() {
    valid_ = true;
  }

public:
  /**
   * \brief Get a local handle from the set
   *
   * \param[in] lookup the local index to look up
   *
   * \return the handle from the set
   */
  HandleType& get(LookupType lookup) {
    vtAssertExpr(valid_);
    auto this_node = static_cast<LookupType>(theContext()->getNode());
    auto idx = IndexType(this_node, lookup);
    auto iter = set_.find(idx);
    vtAssert(iter != set_.end(), "Index must exist here");
    return iter->second;
  }

  /**
   * \brief Operator to get a local handle from the set
   *
   * \param[in] lookup the local index to look up
   *
   * \return the handle from the set
   */
  HandleType& operator[](LookupType lookup) {
    return get(lookup);
  }

  /**
   * \brief Get any of the local handles from the set
   *
   * \return any handle from the set
   */
  HandleType& getAny() {
    vtAssertExpr(valid_);
    auto iter = set_.begin();
    vtAssert(iter != set_.end(), "Index must exist here");
    return iter->second;
  }

  /**
   * \brief Operator to get any of the local handles from the set
   *
   * \return any handle from the set
   */
  HandleType& operator*() {
    return getAny();
  }

  /**
   * \brief Operator to get any of the local handles from the set
   *
   * \return any handle from the set
   */
  HandleType* operator->() {
    vtAssertExpr(valid_);
    auto iter = set_.begin();
    vtAssert(iter != set_.end(), "Index must exist here");
    return &iter->second;
  }

private:
  std::unordered_map<IndexType, HandleType> set_; /**< Holds set of handles */
  bool valid_ = false;
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_SET_H*/
