
#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/base/base.h"
#include "vrt/collection/types/migrate_hooks.h"
#include "vrt/collection/types/migratable.fwd.h"

namespace vt { namespace vrt { namespace collection {

struct Migratable : MigrateHookBase {
  Migratable() = default;

  /*
   * The user or runtime system can invoke this method at any time (when a valid
   * pointer to it exists) to migrate this VCC element to another memory domain
   *
   *  1.  Invoke migrate(node) where node != theContext()->getNode()
   *  2.  Runtime system invokes Migratable::preMigrateOut()
   *  3.  Migratable element is serialized
   *  4.  Migratable element is sent to the destination node
   *  5.  Location manager de-registers this element
   *  6.  VCC manager removes local reference to this element
   *  7.  Runtime system invokes Migratable::epiMigrateOut()
   *  8.  Runtime system invokes Migratable::destroy()
   *  9.  Runtime system invokes the destructor
   *      ....... send MigrateMsg to MigrateHandlers::migrateInHandler .......
   *  10. migrateInHandler() is invoked
   *  11. Migratable element is de-serialized
   *  12. Migratable element constructed with migration constructor
   *  13. Runtime system invokes Migratable::preMigrateIn()
   *  14. VCC manager add local reference to this element (InnerHolder)
   *  15. Location manager registers this element on destination node
   *  16. Runtime system invokes Migratable::epiMigrateIn()
   *
   */
  template <typename ColT>
  void migrate(NodeType const& node);

  void migrateVirtual(NodeType const& node) override {
    return migrate(node);
  }

  /*
   * The system invokes this when the destructor is about to be called on the
   * VCC element due a migrate(NodeType const&) invocation
   */
  virtual void destroy();

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    MigrateHookBase::serialize(s);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H*/
