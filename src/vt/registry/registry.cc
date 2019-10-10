/*
//@HEADER
// *****************************************************************************
//
//                                 registry.cc
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

#include "vt/config.h"
#include "vt/registry/registry.h"

namespace vt { namespace registry {

HandlerType Registry::registerNewHandler(
  ActiveClosureFnType fn, TagType const& tag, bool const& is_collective
) {
  HandlerType new_handle = 0;
  HandlerIdentifierType const& new_identifier =
    is_collective ? cur_ident_collective_++ : cur_ident_++;

  HandlerManagerType::setHandlerIdentifier(new_handle, new_identifier);

  if (tag == no_tag) {
    registered_[new_handle] = fn;
  } else {
    tagged_registered_[new_handle][tag] = fn;
  }

  return new_handle;
}

void Registry::swapHandler(
  HandlerType const& han, ActiveClosureFnType fn, TagType const& tag
) {
  if (tag == no_tag) {
    auto iter = registered_.find(han);
    vtAssert(
      iter != registered_.end(), "Handler must be registered"
    );
    iter->second = fn;
  } else {
    if (fn == nullptr) {
      auto tag_iter = tagged_registered_[han].find(tag);
      if (tag_iter != tagged_registered_[han].end()) {
        tagged_registered_[han].erase(tag_iter);
        if (tagged_registered_[han].size() == 0) {
          tagged_registered_.erase(tagged_registered_.find(han));
        }
      }
    } else {
      tagged_registered_[han][tag] = fn;
    }
  }
}

void Registry::unregisterHandlerFn(
  HandlerType const& han, TagType const& tag
) {
  swapHandler(han, nullptr, tag);
}

HandlerType Registry::registerActiveHandler(
  ActiveClosureFnType fn, TagType const& tag
) {
  return registerNewHandler(fn, tag, true);
}

ActiveClosureFnType Registry::getHandlerNoTag(HandlerType const& han) {
  auto iter = registered_.find(han);
  if (iter != registered_.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

ActiveClosureFnType Registry::getHandler(
  HandlerType const& han, TagType const& tag
) {
  if (tag == no_tag) {
    return getHandlerNoTag(han);
  } else {
    auto tag_iter = tagged_registered_.find(han);
    if (tag_iter == tagged_registered_.end()) {
      return getHandlerNoTag(han);
    } else {
      auto iter = tag_iter->second.find(tag);
      if (iter != tag_iter->second.end()) {
        return iter->second;
      } else {
        return getHandlerNoTag(han);
      }
    }
  }
}

}} //end namespace vt::registry
