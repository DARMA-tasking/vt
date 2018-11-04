
#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_H

#include "vt/config.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#include <tuple>
#include <functional>

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsNoIndex {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(std::get<I>(*tup)...);
  }
};

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsIdxFst {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(idx,std::get<I>(*tup)...);
  }
};

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsIdxSnd {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(std::get<I>(*tup)...,idx);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_H*/
