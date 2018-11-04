
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_IMPL_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/insert/insert_finished.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
InsertFinished<ColT,IndexT,BaseProxyT>::InsertFinished(
  VirtualProxyType const in_proxy
) : BaseProxyT(in_proxy)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
void InsertFinished<ColT,IndexT,BaseProxyT>::finishedInserting(
  ActionType action
) const {
  auto const col_proxy = this->getProxy();
  theCollection()->finishedInserting<ColT,IndexT>(col_proxy,action);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERT_FINISHED_IMPL_H*/
