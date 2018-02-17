
#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_H

#include "config.h"

#include <tuple>
#include <functional>

namespace vt { namespace vrt { namespace collection {

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsNoIndex {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(
      std::forward<typename std::tuple_element<I,Tuple>::type>(
        std::get<I>(*tup)
      )...
    );
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
    return std::make_unique<ColT>(
      idx,
      std::forward<typename std::tuple_element<I,Tuple>::type>(
        std::get<I>(*tup)
      )...
    );
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
    return std::make_unique<ColT>(
      std::forward<typename std::tuple_element<I,Tuple>::type>(
        std::get<I>(*tup)
      )..., idx
    );
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_H*/
