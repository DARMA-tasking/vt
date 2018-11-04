
#if !defined INCLUDED_RDMA_RDMA_COLLECTION_INFO_H
#define INCLUDED_RDMA_RDMA_COLLECTION_INFO_H

#include "vt/config.h"
#include "vt/topos/location/cache/cache.h"
#include "vt/topos/location/record/record.h"

namespace vt { namespace rdma {

namespace loc = ::vt::location;

struct CollectionInfo {
  using ElmType = RDMA_ElmType;
  using LocRecType = loc::LocRecord<ElmType>;
  using LocationCacheType = loc::LocationCache<ElmType, LocRecType>;

private:
  LocationCacheType cache_;
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_RDMA_RDMA_COLLECTION_INFO_H*/
