
#if !defined INCLUDED_LB_INSTRUMENTATION_ENTITY_H
#define INCLUDED_LB_INSTRUMENTATION_ENTITY_H

#include "config.h"
#include "lb/instrumentation/database.h"
#include "lb/lb_types.h"
#include "lb/migration/lb_migratable.h"

#include <unordered_map>

namespace vt { namespace lb { namespace instrumentation {

struct Entity {
  using MigratableType = ::vt::HasMigrate;
  using DatabaseType = Database;

  Entity() = default;

  static LBEntityType registerEntity();
  static LBEntityType registerMigratableEntity(MigratableType* mig);

  static void beginExecution(LBEntityType const& entity);
  static void endExecution(LBEntityType const& entity);
  static void notifyMigrate(NodeType const& node, LBEntityType const& entity);

public:
  static std::unordered_map<LBEntityType, TimeType> events_;
  static std::unordered_map<LBEntityType, DatabaseType> entities_;

private:
  static std::unordered_map<LBEntityType, MigratableType*> migratables_;
  static LBEntityType cur_entity_id;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_ENTITY_H*/
