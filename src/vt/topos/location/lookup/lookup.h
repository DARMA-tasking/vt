/*
//@HEADER
// *****************************************************************************
//
//                                   lookup.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_LOOKUP_LOOKUP_H
#define INCLUDED_VT_TOPOS_LOCATION_LOOKUP_LOOKUP_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/cache/cache.h"
#include "vt/topos/location/directory/directory.h"

namespace vt { namespace location {

template <typename KeyT, typename ValueT>
struct LocLookup {

  LocLookup(LocationSizeType const& in_max_cache_size, NodeType in_this_node)
    : max_cache_size_(in_max_cache_size),
      cache_(in_max_cache_size),
      this_node_(in_this_node)
  { }

  bool exists(KeyT const& key) const;
  LocationSizeType getCacheSize() const;
  ValueT const& get(KeyT const& key);
  void remove(KeyT const& key);
  void insert(KeyT const& key, NodeType const home, ValueT const& value);
  void update(KeyT const& key, ValueT const& value);
  void clearCache();
  void printCache() const;

private:
  LocationSizeType max_cache_size_ = 0;
  Directory<KeyT, ValueT> directory_;
  LocationCache<KeyT, ValueT> cache_;
  NodeType this_node_ = uninitialized_destination;
};

}} /* end namespace vt::location */

#include "vt/topos/location/lookup/lookup.impl.h"

#endif /*INCLUDED_VT_TOPOS_LOCATION_LOOKUP_LOOKUP_H*/
