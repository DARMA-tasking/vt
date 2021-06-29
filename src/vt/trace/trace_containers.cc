/*
//@HEADER
// *****************************************************************************
//
//                             trace_containers.cc
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
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"

#include <memory>

namespace vt { namespace trace {

/*static*/ TraceContainerEventClassType*
TraceContainers::getEventTypeContainer(){
  // n.b. Container MUST be constant-initialized as this function is used
  // from dynamic initialization contexts (ie. auto registrations).
  // static INSIDE function to avoid Clang 3.9 bug; issue not present Clang 5+.
  static std::unique_ptr<TraceContainerEventClassType> event_type_container_ = nullptr;

  if (event_type_container_ == nullptr) {
    event_type_container_ = std::make_unique<TraceContainerEventClassType>();
  }
  return event_type_container_.get();
}

/*static*/ TraceContainerEventType*
TraceContainers::getEventContainer(){
  // n.b. Container MUST be constant-initialized as this function is used
  // from dynamic initialization contexts (ie. auto registrations).
  // static INSIDE function to avoid Clang 3.9 bug; fixed Clang 5+.
  static std::unique_ptr<TraceContainerEventType> event_container_ = nullptr;

  if (event_container_ == nullptr) {
    event_container_ = std::make_unique<TraceContainerEventType>();
  }
  return event_container_.get();
}

}} //end namespace vt::trace
