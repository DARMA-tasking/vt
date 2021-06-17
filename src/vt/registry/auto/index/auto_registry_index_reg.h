/*
//@HEADER
// *****************************************************************************
//
//                          auto_registry_index_reg.h
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

#if !defined INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_REG_H
#define INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_REG_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"

namespace vt { namespace auto_registry {

template <typename=void>
struct MaxIndexHolder {
  static std::size_t index_max_size;
};

template <typename T>
/*static*/ std::size_t MaxIndexHolder<T>::index_max_size = 0;

template <typename IndexT>
struct RegistrarIndex {
  AutoHandlerType index;

  RegistrarIndex();
};

template <typename IndexT>
RegistrarIndex<IndexT>::RegistrarIndex() {
  auto& reg = getAutoRegistryGen<AutoActiveIndexContainerType>();
  index = reg.size();
  MaxIndexHolder<>::index_max_size = std::max(
    MaxIndexHolder<>::index_max_size,
    sizeof(IndexT)
  );
}

template <typename IndexT>
struct RegistrarWrapperIndex {
  RegistrarIndex<IndexT> registrar;
};

template <typename IndexT>
struct IndexHolder {
  static AutoHandlerType const idx;
};

template <typename IndexT>
AutoHandlerType registerIndex() {
  return RegistrarWrapperIndex<IndexT>().registrar.index;
}

template <typename IndexT>
AutoHandlerType const IndexHolder<IndexT>::idx = registerIndex<IndexT>();

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_INDEX_AUTO_REGISTRY_INDEX_REG_H*/
