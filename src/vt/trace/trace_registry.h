/*
//@HEADER
// *****************************************************************************
//
//                               trace_registry.h
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

#if !defined INCLUDED_VT_TRACE_TRACE_REGISTRY_H
#define INCLUDED_VT_TRACE_TRACE_REGISTRY_H

#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"

namespace vt { namespace trace {

struct TraceRegistry {
  /// Registers an event, if it does not already exist.
  /// Creates the event type (aka parent) as needed.
  /// Returns a non-0 value.
  static TraceEntryIDType registerEventHashed(
    std::string const& event_type_name, std::string const& event_name
  );

  /// Changes the name of an event and/or event type name.
  /// Empty string values are ignored, which can be used for partial updates.
  /// Changing the type name will affect ALL events that share the type.
  /// Event trace IDs are not affected.
  static void setTraceName(
    TraceEntryIDType id, std::string const& name, std::string const& type_name
  );

  /// Returns the event that corresponds with the ID.
  /// If not found the returned event has no_trace_entry_id for an ID.
  /// The resulting object is invalidated if new event types are added.
  static EventClassType const& getEvent(TraceEntryIDType id);
};

}} //end namespace vt::trace

#endif /*INCLUDED_VT_TRACE_TRACE_REGISTRY_H*/
