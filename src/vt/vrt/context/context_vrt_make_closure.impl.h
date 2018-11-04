
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_MAKE_CLOSURE_IMPL_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_MAKE_CLOSURE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/context/context_vrt_make_closure.h"

#include <tuple>
#include <utility>
#include <type_traits>
#include <memory>

namespace vt { namespace vrt {

template <typename VrtCtxT, typename Tuple, size_t... I>
/*static*/ VrtPtrType<VrtCtxT> VirtualConstructor::construct(
  Tuple* tup, std::index_sequence<I...>
) {
  return std::make_unique<VrtCtxT>(
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(*tup)
    )...
  );
}

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_MAKE_CLOSURE_IMPL_H*/
