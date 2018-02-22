
#if !defined INCLUDED_LB_INSTRUMENTATION_ENTITY_H
#define INCLUDED_LB_INSTRUMENTATION_ENTITY_H

#include "config.h"
#include "lb/instrumentation/info.h"
#include "lb/lb_types.h"

#include <unordered_map>

namespace vt { namespace lb { namespace instrumentation {

struct Entity {
  using InfoType = Info;

  static LBEntityType registerEntity();

protected:
  std::unordered_map<LBEntityType, InfoType> entities_;

private:
  static LBEntityType cur_entity_id;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_ENTITY_H*/
