/*
//@HEADER
// *****************************************************************************
//
//                              diagnostic.impl.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_IMPL_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_IMPL_H

#include "vt/config.h"
#include "vt/runtime/component/diagnostic.h"
#include "vt/runtime/component/diagnostic_value.h"

#include <memory>

namespace vt { namespace runtime { namespace component {

template <typename T>
void Diagnostic::registerDiagnostic(
  std::string const& key, std::string const& desc, DiagnosticUpdate update,
  DiagnosticTypeEnum type, T initial_value
) {
  vtAssert(values_.find(key) == values_.end(), "Key must not exist");
  values_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(key),
    std::forward_as_tuple(
      std::make_unique<detail::DiagnosticValue<T>>(
        key, desc, update, type, initial_value
      )
    )
  );
}

template <typename T>
void Diagnostic::updateDiagnostic(std::string const& key, T value) {
  auto iter = values_.find(key);
  vtAssert(iter != values_.end(), "Diagnostic key must exist");
  return (static_cast<detail::DiagnosticValue<T>*>(iter->second.get()))->
    update(value);
}

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_IMPL_H*/
