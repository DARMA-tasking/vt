/*
//@HEADER
// *****************************************************************************
//
//                       manager_migrate_attorney.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_MIGRATE_MANAGER_MIGRATE_ATTORNEY_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_MIGRATE_MANAGER_MIGRATE_ATTORNEY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/migrate/manager_migrate_attorney.h"
#include "vt/vrt/collection/migrate/migrate_status.h"
#include "vt/vrt/collection/types/migratable.fwd.h"
#include "vt/vrt/collection/manager.fwd.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ MigrateStatus CollectionElmAttorney<ColT, IndexT>::migrateOut(
  VirtualProxyType const& proxy, IndexT const& idx, NodeT const& dest
) {
  return theCollection()->migrateOut<ColT,IndexT>(proxy,idx,dest);
}


template <typename ColT, typename IndexT>
/*static*/ void CollectionElmAttorney<ColT, IndexT>::migrate(
  VrtElmProxy<ColT, typename ColT::IndexType> proxy, NodeT dest
) {
  theCollection()->migrate<ColT>(proxy,dest);
}

template <typename ColT, typename IndexT>
/*static*/ MigrateStatus CollectionElmAttorney<ColT, IndexT>::migrateIn(
  VirtualProxyType const& proxy, IndexT const& idx, NodeT const& from,
  VirtualPtrType vc_elm
) {
  return theCollection()->migrateIn<ColT,IndexT>(
    proxy, idx, from, std::move(vc_elm)
  );
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_MIGRATE_MANAGER_MIGRATE_ATTORNEY_IMPL_H*/
