
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_H

#include "vt/config.h"
#include "vt/vrt/collection/reducable/reducable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct InsertFinished : BaseProxyT {
  InsertFinished() = default;
  InsertFinished(InsertFinished const&) = default;
  InsertFinished(InsertFinished&&) = default;
  explicit InsertFinished(VirtualProxyType const in_proxy);
  InsertFinished& operator=(InsertFinished const&) = default;

  void finishedInserting(ActionType action = nullptr) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_H*/
