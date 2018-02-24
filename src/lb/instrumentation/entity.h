
#if !defined INCLUDED_LB_INSTRUMENTATION_ENTITY_H
#define INCLUDED_LB_INSTRUMENTATION_ENTITY_H

#include "config.h"
#include "lb/instrumentation/database.h"
#include "lb/lb_types.h"

#include <unordered_map>

namespace vt { namespace lb { namespace instrumentation {

struct Entity {
  using DatabaseType = Database;

  Entity() = default;

  static LBEntityType registerEntity();

  static void beginExecution(LBEntityType const& entity);
  static void endExecution(LBEntityType const& entity);

protected:
  static std::unordered_map<LBEntityType, TimeType> events_;
  static std::unordered_map<LBEntityType, DatabaseType> entities_;

private:
  static LBEntityType cur_entity_id;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_ENTITY_H*/
