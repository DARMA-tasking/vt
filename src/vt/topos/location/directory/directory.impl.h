/*
//@HEADER
// *****************************************************************************
//
//                               directory.impl.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_DIRECTORY_DIRECTORY_IMPL_H
#define INCLUDED_VT_TOPOS_LOCATION_DIRECTORY_DIRECTORY_IMPL_H

#include "vt/config.h"

namespace vt { namespace location {

template <typename KeyT, typename ValueT>
bool Directory<KeyT, ValueT>::exists(KeyT const& key) const {
  auto iter = dir_.find(key);
  return iter != dir_.end();
}

template <typename KeyT, typename ValueT>
ValueT const& Directory<KeyT, ValueT>::get(KeyT const& key) {
  auto iter = dir_.find(key);

  vtAssert(iter != dir_.end(), "Key must exist in directory");

  return iter->second;
}

template <typename KeyT, typename ValueT>
typename Directory<KeyT,ValueT>::DirectoryMapType::iterator
Directory<KeyT, ValueT>::getIter(KeyT const& key) {
  return dir_.find(key);
}

template <typename KeyT, typename ValueT>
typename Directory<KeyT,ValueT>::DirectoryMapType::iterator
Directory<KeyT, ValueT>::getIterEnd() {
  return dir_.end();
}

template <typename KeyT, typename ValueT>
LocationSizeType Directory<KeyT, ValueT>::getSize() const {
  return dir_.size();
}

template <typename KeyT, typename ValueT>
void Directory<KeyT, ValueT>::remove(KeyT const& key) {
  auto iter = dir_.find(key);
  if (iter != dir_.end()) {
    dir_.erase(iter);
  }
}

template <typename KeyT, typename ValueT>
void Directory<KeyT, ValueT>::insert(KeyT const& key, ValueT const& value) {
  auto iter = dir_.find(key);

  if (iter == dir_.end()) {
    dir_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(value)
    );
  } else {
    iter->second = value;
  }
}

}} /* end namespace vt::location */

#endif /*INCLUDED_VT_TOPOS_LOCATION_DIRECTORY_DIRECTORY_IMPL_H*/
