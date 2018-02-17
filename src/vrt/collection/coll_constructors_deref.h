
#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H

#include "config.h"
#include "vrt/collection/manager.h"

#include <tuple>
#include <functional>

namespace vt { namespace vrt { namespace collection {

struct DerefCons {
  template <typename ColT, typename IndexT, typename Tuple, typename... Args>
  static typename CollectionManager::VirtualPtrType<IndexT>
  derefTuple(
    VirtualElmCountType const& elms, IndexT const& idx, std::tuple<Args...>* tup
  );

  template <
    typename ColT, typename IndexT, typename Tuple, typename DispatcherT,
    size_t... I
  >
  static typename CollectionManager::VirtualPtrType<IndexT>
  expandSeq(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...> seq
  );
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/coll_constructors_deref.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H*/
