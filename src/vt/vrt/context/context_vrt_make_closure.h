
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_MAKE_CLOSURE_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_MAKE_CLOSURE_H

#include "vt/config.h"
#include "vt/vrt/context/context_vrt_fwd.h"
#include "vt/vrt/context/context_vrt_attorney.h"
#include "vt/vrt/context/context_vrtproxy.h"
#include "vt/vrt/context/context_vrtinfo.h"

#include <tuple>
#include <utility>
#include <type_traits>
#include <memory>

namespace vt { namespace vrt {

template <typename VrtCtxT>
using VrtPtrType = std::unique_ptr<VrtCtxT>;

struct VirtualConstructor {
  template <typename VrtCtxT, typename Tuple, size_t... I>
  static VrtPtrType<VrtCtxT> construct(Tuple* tup, std::index_sequence<I...>);
};

template <typename VrtContextT, typename... Args>
struct VirtualMakeClosure {
  using TupleType = std::tuple<Args...>;

  TupleType tup;
  VirtualProxyType proxy = no_vrt_proxy;
  VirtualInfo* info = nullptr;

  VirtualMakeClosure(
    TupleType&& in_tup, VirtualProxyType const& in_proxy, VirtualInfo* in_info
  ) : tup(std::forward<TupleType>(in_tup)), proxy(in_proxy), info(in_info)
  { }
  VirtualMakeClosure(VirtualMakeClosure&&) = default;

  void make() {
    debug_print(
      vrt, node,
      "VirtualMakeClosure: calling make()\n"
    );

    static constexpr auto args_size = std::tuple_size<TupleType>::value;
    auto vc_ptr = VirtualConstructor::construct<VrtContextT>(
      &tup, std::make_index_sequence<args_size>{}
    );

    VirtualContextAttorney::setProxy(vc_ptr.get(), proxy);
    info->setVirtualContextPtr(std::move(vc_ptr));
  }
};

}} /* end namespace vt::vrt */

#include "vt/vrt/context/context_vrt_make_closure.impl.h"

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_MAKE_CLOSURE_H*/
