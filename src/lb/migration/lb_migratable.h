
#if !defined INCLUDED_LB_MIGRATION_LB_MIGRATABLE_H
#define INCLUDED_LB_MIGRATION_LB_MIGRATABLE_H

#include "config.h"

namespace vt {

struct LBMigratable {
  virtual void migrateVirtual(NodeType const& to_node) = 0;
};

} /* end namespace vt */

#endif /*INCLUDED_LB_MIGRATION_LB_MIGRATABLE_H*/
