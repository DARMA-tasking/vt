/*
//@HEADER
// ************************************************************************
//
//                          lb_default_migrate.cc
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

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/lb_types_internal.h"
#include "vt/lb/migration/migrate.h"
#include "vt/lb/instrumentation/entity.h"
#include "vt/lb/balancers/centralized/lb_interface.h"
#include "vt/lb/balancers/centralized/lb_default_migrate.h"
#include "vt/context/context.h"

namespace vt { namespace lb { namespace centralized {

/*virtual*/ void CentralMigrate::notifyMigration(
  NodeType const& from, NodeType const& to, LBEntityType const& entity
) {
  auto const& this_node = theContext()->getNode();
  if (this_node == from) {
    migrate(to, entity);
  }
}

/*virtual*/ void CentralMigrate::notifyMigrationList(
  MigrateInfoType const& migrate_info
) {
  auto const& this_node = theContext()->getNode();
  for (auto&& elm : migrate_info.migrations_) {
    if (elm.first == this_node) {
      for (auto&& migrate_to_list : elm.second) {
        for (auto&& entity : migrate_to_list.second) {
          migrate(migrate_to_list.first, entity);
        }
      }
    }
  }
}

/*virtual*/ void CentralMigrate::finishedMigrations() {
  debug_print(
    lb, node,
    "CentralMigrate::finishedMigrations: before sync\n"
  );

  sync();

  debug_print(
    lb, node,
    "CentralMigrate::finishedMigrations: after sync\n"
  );
}

void CentralMigrate::migrate(
  NodeType const& to_node, LBEntityType const& entity
) {
  return instrumentation::Entity::notifyMigrate(to_node, entity);
}

}}} /* end namespace vt::lb::centralized */

