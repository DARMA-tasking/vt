/*
//@HEADER
// *****************************************************************************
//
//                                 stats_lb_reader.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_STATS_LB_READER_H
#define INCLUDED_VRT_COLLECTION_BALANCE_STATS_LB_READER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/objgroup/headers.h"

#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <vector>


namespace vt { namespace vrt { namespace collection { namespace lb {

template<typename Transfer>
struct TransferMsg;

using VecMsg = TransferMsg< std::vector< balance::ElementIDType > >;

} } } }


namespace vt { namespace vrt { namespace collection { namespace balance {

struct StatsLBReader {

public:
  StatsLBReader() = default;
  StatsLBReader(StatsLBReader const&) = delete;
  StatsLBReader(StatsLBReader&&) = default;

  static void init();
  static void destroy();
  static void clearStats();
  static void inputStatsFile();
  static void loadPhaseChangedMap();

public:

  /// \brief Queue to store a map of elements specified by input file.
  static std::deque< std::set<ElementIDType> > user_specified_map_;

  /// \brief Vector of booleans to indicate whether the user-specified
  /// map migrates some elements for a specific iteration.
  static std::vector<bool> phase_changed_map_;

  /// \brief Queue of migrations for each iteration.
  /// \note At each iteration, a vector of length 2 times (# of migrations)
  /// is specified. The vector contains the "permanent" ID of the element
  /// to migrate followed by the node ID to migrate to.
  static std::deque< std::vector<ElementIDType> > moveList;

protected:

  void doSend(lb::VecMsg *msg);
  void scatterSend(lb::VecMsg *msg);

  /// \brief Vector counting the received messages per iteration
  /// \note Only node 0 will use this vector.
  static std::vector<size_t> msgReceived;

  /// \brief Queue for storing all the migrations per iteration.
  /// \note Only node 0 will use this queue.
  static std::deque<std::map<ElementIDType, std::pair<NodeType, NodeType>>>
    totalMove;

  static objgroup::proxy::Proxy<StatsLBReader> proxy_;

};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_STATS_LB_READER_H*/
