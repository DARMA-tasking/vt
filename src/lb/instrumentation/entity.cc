
#include "config.h"
#include "context/context.h"
#include "lb/instrumentation/entity.h"

#include <cassert>

namespace vt { namespace lb { namespace instrumentation {

/*static*/ LBEntityType Entity::cur_entity_id = 0;

/*static*/ LBEntityType Entity::registerEntity() {
  auto const& node = theContext()->getNode();
  auto const& next_id = cur_entity_id++;
  return next_id | (static_cast<LBEntityType>(node) << 48);
}

/*static*/ LBEntityType Entity::registerMigratableEntity(MigratableType* mig) {
  auto const& entity = registerEntity();
  migratables_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(entity),
    std::forward_as_tuple(mig)
  );
  return entity;
}

/*static*/ void Entity::notifyMigrate(
  NodeType const& to_node, LBEntityType const& entity
) {
  auto iter = migratables_.find(entity);
  vtAssert(
    iter != migratables_.end(), "Entity must exist in migratables to migrate"
  );
  auto const& elm = iter->second;
  elm->migrate(to_node);
  migratables_.erase(iter);
}

/*static*/ void Entity::beginExecution(LBEntityType const& entity) {
  auto const& current_time = timing::Timing::getCurrentTime();
  auto event_iter = events_.find(entity);
  if (event_iter == events_.end()) {
    events_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(entity),
      std::forward_as_tuple(current_time)
    );
  } else {
    event_iter->second = current_time;
  }
}

/*static*/ void Entity::endExecution(LBEntityType const& entity) {
  auto entity_iter = entities_.find(entity);
  if (entity_iter == entities_.end()) {
    entities_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(entity),
      std::forward_as_tuple(DatabaseType{})
    );
    entity_iter = entities_.find(entity);
  }
  auto const& event_iter = events_.find(entity);
  vtAssert(
    event_iter != events_.end(), "Must have begin"
  );
  auto const& begin_time = event_iter->second;
  auto const& end_time = timing::Timing::getCurrentTime();
  entity_iter->second.addEntry(Entry{begin_time,end_time});
}

}}} /* end namespace vt::lb::instrumentation */
