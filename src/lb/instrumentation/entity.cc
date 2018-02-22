
#include "config.h"
#include "context/context.h"
#include "lb/instrumentation/entity.h"

namespace vt { namespace lb { namespace instrumentation {

/*static*/ LBEntityType Entity::cur_entity_id = 0;

/*static*/ LBEntityType Entity::registerEntity() {
  auto const& node = theContext()->getNode();
  auto const& next_id = cur_entity_id++;
  return next_id | (static_cast<LBEntityType>(node) << 48);
}

}}} /* end namespace vt::lb::instrumentation */
