/*
//@HEADER
// *****************************************************************************
//
//                               example_events.h
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

#if !defined INCLUDED_VT_METRICS_EXAMPLE_EVENTS_H
#define INCLUDED_VT_METRICS_EXAMPLE_EVENTS_H

#include "vt/metrics/perf_event_descriptor.h"

#include <unordered_map>
#include <linux/perf_event.h>

namespace vt { namespace metrics {

std::unordered_map<std::string, PerfEventDescriptor> const example_event_map = {
    {"cycles", PerfEventDescriptor{PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "compute"}},
    {"instructions", PerfEventDescriptor{PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "compute"}},
    {"cache_references", PerfEventDescriptor{PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, "cache"}},
    {"cache_misses", PerfEventDescriptor{PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, "cache"}},
    {"branch_instructions", PerfEventDescriptor{PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, "branch"}},
    {"branch_misses", PerfEventDescriptor{PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, "branch"}}
};

}} // end namespace vt::metrics

#endif /*INCLUDED_VT_METRICS_EXAMPLE_EVENTS_H*/
