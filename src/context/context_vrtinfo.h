
#if !defined __RUNTIME_TRANSPORT_CONTEXT_VRTINFO__
#define __RUNTIME_TRANSPORT_CONTEXT_VRTINFO__

#include "config.h"
#include "context_vrt.h"

#include <memory>
#include <cassert>

namespace vt { namespace vrt {

struct VrtInfo {
  using VrtContextPtrType = std::unique_ptr<VrtContext>;

  VrtInfo(VrtContextPtrType in_vrt_ptr)
    : vrt_ptr_(std::move(in_vrt_ptr))
  { }
  VrtInfo(VrtInfo&&) = default;
  VrtInfo(VrtInfo const&) = delete;

  void mapToCore(CoreType const& core) {
    default_core_ = core;
  }

  VrtContext* get() const {
    assert(vrt_ptr_ != nullptr and "Must have a valid context");
    return vrt_ptr_.get();
  }

  CoreType getCore() const {
    return default_core_;
  }

private:
  CoreType default_core_ = uninitialized_destination;

  VrtContextPtrType vrt_ptr_ = nullptr;
};

}} /* end namespace vt::vrt */

#endif /*__RUNTIME_TRANSPORT_CONTEXT_VRTINFO__*/
