/*
//@HEADER
// *****************************************************************************
//
//                                  perf_event_map.h
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

#if !defined INCLUDED_VT_METRICS_PERF_MAP_H
#define INCLUDED_VT_METRICS_PERF_MAP_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "example_events.h"

namespace vt {  namespace metrics {

/** \file */

/**
 * \struct PerfEventMap perf_event_map.h vt/metrics/perf_event_map.h
 *
 * \brief Used to obtain the association between string names of metrics and their corresponding perf event type and identifier
 *
 */
struct PerfEventMap : runtime::component::Component<PerfEventMap> {
  /**
   * \brief Gets the map of event names to their corresponding perf variables
   * used.
   *
   * \return the node currently being run on
   */
  std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> getEventMap() const { return event_map_; }

  void startup() override { event_map_ = example_event_map; }

  std::string name() override { return "PerfEventMap"; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | event_map_;
  }

private:
  std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> event_map_ = {};
};

}} // end namespace vt::metrics

namespace vt {

extern metrics::PerfEventMap* thePerfEventMap();

} // end namespace vt

#endif /*INCLUDED_VT_METRICS_PERF_MAP_H*/
