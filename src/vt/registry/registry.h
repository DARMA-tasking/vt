/*
//@HEADER
// ************************************************************************
//
//                          registry.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_REGISTRY_REGISTRY_H
#define INCLUDED_REGISTRY_REGISTRY_H

#include <vector>
#include <unordered_map>
#include <cassert>

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/handler/handler.h"

namespace vt { namespace registry {

struct Registry {
  using HandlerManagerType = HandlerManager;
  using HandlerBitsType = eHandlerBits;
  using TaggerHandlerType = std::tuple<TagType, HandlerType>;
  using ContainerType = std::unordered_map<HandlerType, ActiveClosureFnType>;
  using TagContainerType = std::unordered_map<TagType, ActiveClosureFnType>;
  using HanTagContainerType = std::unordered_map<HandlerType, TagContainerType>;

  Registry() = default;

  HandlerType registerNewHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag,
    bool const& is_collective = false
  );

  void unregisterHandlerFn(
    HandlerType const& han, TagType const& tag = no_tag
  );
  void swapHandler(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  HandlerType registerActiveHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  ActiveClosureFnType getHandler(
    HandlerType const& han, TagType const& tag = no_tag
  );
  ActiveClosureFnType getHandlerNoTag(HandlerType const& han);
  ActiveClosureFnType getTrigger(HandlerType const& han);

  void saveTrigger(HandlerType const& han, ActiveClosureFnType fn);

private:
  ContainerType triggers_;
  ContainerType registered_;
  HanTagContainerType tagged_registered_;
  HandlerIdentifierType cur_ident_collective_ = first_handle_identifier;
  HandlerIdentifierType cur_ident_ = first_handle_identifier;
};

}} //end namespace vt::registry

namespace vt {

extern registry::Registry* theRegistry();

} // end namespace vt

#endif /*INCLUDED_REGISTRY_REGISTRY_H*/
