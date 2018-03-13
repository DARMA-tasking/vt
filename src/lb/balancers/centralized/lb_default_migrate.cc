
#include "config.h"
#include "lb/lb_types.h"
#include "lb/lb_types_internal.h"
#include "lb/migration/migrate.h"
#include "lb/balancers/centralized/lb_interface.h"
#include "lb/balancers/centralized/lb_default_migrate.h"
#include "context/context.h"

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
  return Entity::notifyMigrate(to_node, entity);
}

}}} /* end namespace vt::lb::centralized */

