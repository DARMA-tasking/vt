
#if !defined __RUNTIME_TRANSPORT_CONTEXT_VRTINFO__
#define __RUNTIME_TRANSPORT_CONTEXT_VRTINFO__

#include "config.h"
#include "context_vrt.h"

#include <memory>
#include <cassert>

namespace vt { namespace vrt {

struct VirtualInfo {
  using VirtualPtrType = std::unique_ptr<VirtualContext>;

  VirtualInfo(VirtualPtrType in_vrt_ptr, VirtualProxyType const& proxy_in)
    : proxy_(proxy_in), vrt_ptr_(std::move(in_vrt_ptr))
  { }
  VirtualInfo(VirtualInfo&&) = default;
  VirtualInfo(VirtualInfo const&) = delete;

  void mapToCore(CoreType const& core) {
    default_core_ = core;
  }

  VirtualContext* get() const {
    assert(vrt_ptr_ != nullptr and "Must have a valid context");
    return vrt_ptr_.get();
  }

  CoreType getCore() const {
    return default_core_;
  }

  VirtualProxyType getProxy() const {
    return proxy_;
  }

private:
  HandlerType core_map_handle_ = uninitialized_handler;
  HandlerType node_map_handle_ = uninitialized_handler;

  CoreType default_core_ = uninitialized_destination;

  VirtualProxyType proxy_ = no_vrt_proxy;

  VirtualPtrType vrt_ptr_ = nullptr;
};

}} /* end namespace vt::vrt */

#endif /*__RUNTIME_TRANSPORT_CONTEXT_VRTINFO__*/
