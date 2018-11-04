
#if !defined INCLUDED_LB_BALANCERS_CENTRALIZED_LB_DEFAULT_MIGRATE_H
#define INCLUDED_LB_BALANCERS_CENTRALIZED_LB_DEFAULT_MIGRATE_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/lb_types_internal.h"
#include "vt/lb/migration/migrate.h"
#include "vt/lb/balancers/centralized/lb_interface.h"

namespace vt { namespace lb { namespace centralized {

struct CentralMigrate : CentralLB {

  CentralMigrate(NodeType const& node, NodeType const& central_node)
    : CentralLB(node, central_node)
  { }

  virtual void notifyMigration(
    NodeType const& from, NodeType const& to, LBEntityType const& entity
  ) override;
  virtual void notifyMigrationList(MigrateInfoType const& migrate_info) override;
  virtual void finishedMigrations() override;

protected:
  void migrate(NodeType const& to_node, LBEntityType const& entity);
};

}}} /* end namespace vt::lb::centralized */

#endif /*INCLUDED_LB_BALANCERS_CENTRALIZED_LB_DEFAULT_MIGRATE_H*/
