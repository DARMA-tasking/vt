
#if !defined INCLUDED_LB_MIGRATION_MIGRATE_H
#define INCLUDED_LB_MIGRATION_MIGRATE_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"

#include <unordered_map>
#include <vector>

namespace vt { namespace lb {

struct MigrateInfo {
  using MigrateToType = NodeType;
  using MigrateFromType = NodeType;
  using MigrateListType = std::vector<LBEntityType>;
  using MigrateToListType = std::unordered_map<MigrateToType, MigrateListType>;
  using MigrateFromListType = std::unordered_map<MigrateFromType, MigrateToListType>;

  MigrateInfo() = default;
  explicit MigrateInfo(MigrateFromListType const& in_migrations)
    : migrations_(in_migrations)
  { }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | migrations_;
  }

  MigrateFromListType migrations_;
};

}} /* end namespace vt::lb */

#endif /*INCLUDED_LB_MIGRATION_MIGRATE_H*/
