
#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#include <tuple>
#include <functional>

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

struct DerefCons {
  template <typename ColT, typename IndexT, typename Tuple, typename... Args>
  static typename CollectionManager::VirtualPtrType<ColT, IndexT>
  derefTuple(
    VirtualElmCountType const& elms, IndexT const& idx, std::tuple<Args...>* tup
  );

  template <
    typename ColT, typename IndexT, typename Tuple, typename DispatcherT,
    size_t... I
  >
  static typename CollectionManager::VirtualPtrType<ColT, IndexT>
  expandSeq(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...> seq
  );
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/constructor/coll_constructors_deref.impl.h"

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H*/
