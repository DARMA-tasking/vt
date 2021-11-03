/*
//@HEADER
// *****************************************************************************
//
//                                 datastore.h
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

#if !defined INCLUDED_VT_DATAREP_DATASTORE_H
#define INCLUDED_VT_DATAREP_DATASTORE_H

#include <memory>
#include <unordered_map>
#include <set>

namespace vt { namespace datarep {

struct DataStoreBase {
  virtual ~DataStoreBase() = default;
};

template <typename T, typename IndexT>
struct DataStore final : DataStoreBase {
  using NodeSetType     = std::set<NodeType>;
  using VersionMapType  = std::unordered_map<DataVersionType, std::shared_ptr<T>>;
  using VersionNodeType = std::unordered_map<DataVersionType, NodeSetType>;

  explicit DataStore(
    bool in_is_master, IndexT idx, DataVersionType version,
    std::shared_ptr<T> data
  ) : is_master(in_is_master)
  {
    cache_[idx][version] = data;
  }

  void const* get(IndexT idx, DataVersionType version) const {
    auto iter = cache_.find(idx);
    return static_cast<void const*>(iter->second.find(version)->second.get());
  }

  std::shared_ptr<T> getSharedPtr(IndexT idx, DataVersionType version) const {
    auto iter = cache_.find(idx);
    if (iter != cache_.end()) {
      auto iter2 = iter->second.find(version);
      if (iter2 != iter->second.end()) {
        return iter2->second;
      }
    }
    return nullptr;
  }

  void publishVersion(
    IndexT idx, DataVersionType version, std::shared_ptr<T> data
  ) {
    cache_[idx][version] = data;
  }

  void unpublishVersion(IndexT idx, DataVersionType version) {
    auto iter = cache_.find(idx);
    if (iter != cache_.end()) {
      auto iter2 = iter->second.find(version);
      if (iter2 != iter->second.end()) {
        iter->second.erase(iter2);
      }
      if (iter->second.size() == 0) {
        cache_.erase(iter);
      }
    }
  }

  void recordRequest(IndexT idx, DataVersionType version, NodeType requestor) {
    request_nodes_[idx][version].insert(requestor);
  }

  NodeSetType const& getRequestors(IndexT idx, DataVersionType version) const {
    auto iter = request_nodes_.find(idx);
    if (iter != request_node_.end()) {
      auto version_iter = iter->second.find(version);
      if (version_iter != iter->second.end()) {
        return version_iter->second;
      }
    }

    std::set<NodeType> empty_set;
    return empty_set;
  }

  bool hasVersion(IndexT idx, DataVersionType version) const {
    auto iter = cache_.find(idx);
    return iter != cache_.end() and
           iter->second.find(version) != iter->second.end();
  }

private:
  /// Whether this is the master copy or not
  bool is_master = false;
  /// The actual data (either cached or the original copy)
  std::unordered_map<IndexT, VersionMapType> cache_          = {};
  /// Information about nodes that requested the data for each version
  std::unordered_map<IndexT, VersionNodeType> request_nodes_ = {};
};

}} /* end namespace vt::datarep */

#endif /*INCLUDED_VT_DATAREP_DATASTORE_H*/
