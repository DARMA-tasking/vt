
#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/migratable.h"
#include "vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

/*virtual*/ void Migratable::destroy() {
  debug_print(
    vrt_coll, node,
    "Migratable::destroy(): this=%p\n", this
  );
}

}}} /* end namespace vt::vrt::collection */
