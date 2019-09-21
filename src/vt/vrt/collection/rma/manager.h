/*
//@HEADER
// *****************************************************************************
//
//                                  manager.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_RMA_MANAGER_H
#define INCLUDED_VT_VRT_COLLECTION_RMA_MANAGER_H

#include "vt/config.h"
#include "vt/vrt/collection/rma/connect_msg.h"
#include "vt/vrt/collection/rma/connected_msg.h"
#include "vt/vrt/collection/rma/count_msg.h"

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <memory>
#include <tuple>

namespace vt { namespace vrt { namespace collection { namespace rma {

struct Payload {
  Payload(MPI_Win in_window, int in_count, void* in_ptr)
    : window(in_window), count(in_count), ptr(in_ptr)
  { }

  MPI_Win window;
  int count = 0;
  void* ptr = nullptr;
};

struct Manager {
  template <typename ColT>
  static void connect(ConnectMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  static void connected(ConnectedMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  static void finish(HandleType handle);

  template <typename ColT>
  static void addLocalHandle(HandleType handle, typename ColT::IndexType idx);

  template <typename ColT>
  static void finishLocalCount(CountMsg<ColT>* msg);

  template <typename ColT, typename T>
  static void finishRankMap(RankCountMsg<ColT, T>* msg);

  template <typename ColT>
  static int atomicGetAccum(HandleType handle, int rank, int slot, int val);

private:
  static std::unordered_map<HandleType, int> handle_slot;

  template <typename IndexT>
  static std::unordered_map<HandleType, std::map<IndexT, int>> local_offsets;

  static std::unordered_map<HandleType, VirtualProxyType> handle_to_proxy;

  template <typename IndexT>
  static std::unordered_map<HandleType, std::unordered_set<IndexT>> local_handles;

  template <typename ColT>
  static std::unordered_map<HandleType, std::unique_ptr<Payload>> payload_;

  template <typename IndexT>
  static std::unordered_map<
    HandleType,
    std::unordered_map<
      IndexT,
      std::vector<std::tuple<IndexT, NodeType>>
    >
  > remote_accessors;
};

}}}} /* end namespace vt::vrt::collection::rma */

#include "vt/vrt/collection/rma/manager.impl.h"

#endif /*INCLUDED_VT_VRT_COLLECTION_RMA_MANAGER_H*/
