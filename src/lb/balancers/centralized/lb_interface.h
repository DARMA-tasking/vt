
#if !defined INCLUDED_LB_BALANCERS_CENTRALIZED_LB_INTERFACE_H
#define INCLUDED_LB_BALANCERS_CENTRALIZED_LB_INTERFACE_H

#include "config.h"
#include "lb/lb_types.h"
#include "lb/lb_types_internal.h"
#include "lb/migration/migrate.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace lb { namespace centralized {

struct CentralLB {
  using LoadStatsType = ::vt::lb::ProcContainerType;
  using MigrateInfoType = ::vt::lb::MigrateInfo;

  CentralLB(NodeType const& node, NodeType const& central_node)
    : this_node_(node), central_node_(central_node)
  { }
  CentralLB(CentralLB const&) = delete;
  CentralLB(CentralLB&&) = delete;

  virtual void loadStatistics(LoadStatsType const& stats) = 0;
  virtual void sync() = 0;
  virtual void runLB() = 0;
  virtual void notifyMigration(
    NodeType const& from, NodeType const& to, LBEntityType const& entity
  ) = 0;
  virtual void notifyMigrationList(MigrateInfoType const& migrate_info) = 0;
  virtual void finishedMigrations() = 0;

  NodeType getCentralNode() const { return central_node_; }
  NodeType getThisNode() const { return this_node_; }

private:
  NodeType const this_node_;
  NodeType const central_node_;
};

}}} /* end namespace vt::lb::centralized */

#endif /*INCLUDED_LB_BALANCERS_CENTRALIZED_LB_INTERFACE_H*/
