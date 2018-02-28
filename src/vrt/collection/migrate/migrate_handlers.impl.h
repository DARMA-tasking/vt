
#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_IMPL_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/migrate/migrate_msg.h"
#include "vrt/collection/migrate/migrate_handlers.h"
#include "vrt/collection/migrate/manager_migrate_attorney.h"
#include "serialization/serialization.h"

#include <memory>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ void MigrateHandlers::migrateInHandler(
  MigrateMsg<ColT, IndexT>* msg
) {
  auto const& full_proxy = msg->getElementProxy();
  auto const& col_proxy = full_proxy.getCollectionProxy();
  auto const& elm_proxy = full_proxy.getElementProxy();
  auto const& from_node = msg->getFromNode();
  auto const& idx = elm_proxy.getIndex();

  auto buf = reinterpret_cast<SerialByteType*>(msg->getPut());
  auto const& buf_size = msg->getPutSize();
  auto vc_elm_ptr = ::serialization::interface::deserialize<ColT>(
    ::serialization::interface::serdes_unique_tag{}, buf, buf_size
  );
  using UniquePtrType = decltype(vc_elm_ptr);

  // auto vc_raw_ptr = ::serialization::interface::deserialize<ColT>(
  //   buf, buf_size
  // );
  // auto vc_elm_ptr = std::make_unique<ColT>(vc_raw_ptr, [](ColT* to_delete){
  //   auto char_type_pointer = reinterpret_cast<SerialByteType*>(to_delete);
  //   delete [] char_type_pointer;
  // });

  auto const& migrate_status =
    CollectionElmAttorney<ColT,IndexT>::template migrateIn<UniquePtrType>(
      col_proxy, idx, from_node, std::move(vc_elm_ptr)
    );

  assert(
    migrate_status == MigrateStatus::MigrateInLocal &&
    "Should be valid local migration into this memory domain"
  );
}

}}} /* end namespace vt::vrt::collection */


#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_IMPL_H*/
