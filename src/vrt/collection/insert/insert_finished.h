
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_H

#include "config.h"
#include "vrt/collection/reducable/reducable.h"
#include "vrt/proxy/base_wrapper.h"
#include "activefn/activefn.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct InsertFinished : Reducable<ColT, IndexT> {
  InsertFinished() = default;
  InsertFinished(InsertFinished const&) = default;
  InsertFinished(InsertFinished&&) = default;
  explicit InsertFinished(VirtualProxyType const in_proxy);
  InsertFinished& operator=(InsertFinished const&) = default;

  void finishedInserting(ActionType action = nullptr) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_H*/
