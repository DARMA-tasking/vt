
#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_IMPL_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_IMPL_H

#include "config.h"
#include "vrt/collection/manager.h"

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename Tuple, typename... Args>
/*static*/ typename CollectionManager::VirtualPtrType<ColT, IndexT>
DerefCons::derefTuple(
  VirtualElmCountType const& elms, IndexT const& idx, std::tuple<Args...>* tup
) {
  static constexpr auto size = std::tuple_size<Tuple>::value;
  static constexpr auto seq = std::make_index_sequence<size>{};
  using DispatcherType = DispatchCons<
    ColT, IndexT, typename CollectionManager::VirtualPtrType<ColT, IndexT>,
    Tuple, Args...
  >;
  return expandSeq<ColT, IndexT, Tuple, DispatcherType>(elms, idx, tup, seq);
}

template <
  typename ColT, typename IndexT, typename Tuple, typename DispatcherT,
  size_t... I
>
/*static*/ typename CollectionManager::VirtualPtrType<ColT, IndexT>
DerefCons::expandSeq(
  VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
  std::index_sequence<I...> seq
) {
  using ConsType = typename DispatcherT::template ConsType<I...>;
  ConsType cons;
  return cons(elms, idx, tup, seq);
}

}}} /* end namespace vt::vrt::collection */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_IMPL_H*/
