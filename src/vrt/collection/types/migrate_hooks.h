
#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/untyped.h"

namespace vt { namespace vrt { namespace collection {

struct MigrateHookInterface : UntypedCollection {
  MigrateHookInterface() = default;

public:
  virtual void preMigrateOut() = 0;
  virtual void epiMigrateOut() = 0;
  virtual void preMigrateIn()  = 0;
  virtual void epiMigrateIn()  = 0;

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    UntypedCollection::serialize(s);
  }
};

struct MigrateHookBase : MigrateHookInterface {
  MigrateHookBase() = default;

public:
  virtual void preMigrateOut() override {
    debug_print(vrt_coll, node, "preMigrateOut(): this={}\n", print_ptr(this));
  }
  virtual void epiMigrateOut() override {
    debug_print(vrt_coll, node, "epiMigrateOut(): this={}\n", print_ptr(this));
  }
  virtual void preMigrateIn() override {
    debug_print(vrt_coll, node, "preMigrateIn(): this={}\n", print_ptr(this));
  }
  virtual void epiMigrateIn() override {
    debug_print(vrt_coll, node, "epiMigrateIn(): this={}\n", print_ptr(this));
  }

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    MigrateHookInterface::serialize(s);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H*/
