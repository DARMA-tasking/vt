
#if !defined INCLUDED_VRT_COLLECTION_TRAITS_CONS_DISPATCH_H
#define INCLUDED_VRT_COLLECTION_TRAITS_CONS_DISPATCH_H

#include "vt/config.h"
#include "vt/vrt/collection/traits/cons_detect.h"
#include "vt/vrt/collection/constructor/coll_constructors.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /* backend_check_enabled(detector) */

#include <functional>
#include <tuple>

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

template <
  typename ColT, typename IndexT, typename RetT, typename TupleT,
  typename Enable, typename... Args
>
struct DispatchConsImpl;

/*
 * Constructor pattern without index
 */

template <
  typename ColT, typename IndexT, typename RetT, typename TupleT,
  typename... Args
>
struct DispatchConsImpl<
  ColT, IndexT, RetT, TupleT,
  typename std::enable_if_t<
    ConstructorType<ColT,IndexT,Args...>::use_no_index
  >, Args...
> {
  template <size_t... I>
  using ConsType = DetectConsNoIndex<ColT, IndexT, TupleT, RetT, I...>;
};

/*
 * Constructor pattern with index preceding the constructor arguments
 */

template <
  typename ColT, typename IndexT, typename RetT, typename TupleT,
  typename... Args
>
struct DispatchConsImpl<
  ColT, IndexT, RetT, TupleT,
  typename std::enable_if_t<
    ConstructorType<ColT,IndexT,Args...>::use_index_fst
  >, Args...
> {
  template <size_t... I>
  using ConsType = DetectConsIdxFst<ColT, IndexT, TupleT, RetT, I...>;
};

/*
 * Constructor pattern with index following the constructor arguments
 */

template <
  typename ColT, typename IndexT, typename RetT, typename TupleT,
  typename... Args
>
struct DispatchConsImpl<
  ColT, IndexT, RetT, TupleT,
  typename std::enable_if_t<
    ConstructorType<ColT,IndexT,Args...>::use_index_snd
  >, Args...
> {
  template <size_t... I>
  using ConsType = DetectConsIdxSnd<ColT, IndexT, TupleT, RetT, I...>;
};

template <
  typename ColT, typename IndexT, typename RetT, typename TupleT,
  typename... Args
>
struct DispatchCons : DispatchConsImpl<
  ColT, IndexT, RetT, TupleT, void, Args...
>{ };

}}} /* end namespace vt::vrt::collection */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_TRAITS_CONS_DISPATCH_H*/
