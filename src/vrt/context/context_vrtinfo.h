
#if !defined INCLUDED_CONTEXT_VRT_INFO
#define INCLUDED_CONTEXT_VRT_INFO

#include "config.h"
#include "vrt/context/context_vrt.h"
#include "vrt/context/context_vrtmessage.h"
#include "vrt/context/context_vrt_fwd.h"
#include "utils/mutex/mutex.h"
#include "utils/atomic/atomic.h"
#include "utils/container/process_ready_buffer.h"
#include "registry/auto/auto_registry_vc.h"
#include "registry/auto/auto_registry_map.h"
#include "worker/worker_headers.h"

#include <memory>
#include <cassert>
#include <vector>

namespace vt { namespace vrt {

using ::vt::util::atomic::AtomicType;
using ::vt::util::container::ProcessBuffer;

struct VirtualInfo {
  using MsgBufferContainerType = ProcessBuffer<VirtualMessage*>;
  using VirtualPtrType = std::unique_ptr<VirtualContext>;

  VirtualInfo(
    VirtualPtrType in_vrt_ptr, VirtualProxyType const& proxy_in, bool needs_lock
  );

  VirtualInfo(VirtualInfo&&) = default;
  VirtualInfo(VirtualInfo const&) = delete;

  template <typename VrtContextT, typename... Args>
  friend struct VirtualMakeClosure;

  void setVirtualContextPtr(VirtualPtrType in_vrt_ptr);
  bool enqueueWorkUnit(VirtualMessage* msg);
  void tryEnqueueWorkUnit(VirtualMessage* msg);

  VirtualContext *get() const;

  bool isConstructed() const { return is_constructed_.load(); }
  VirtualProxyType getProxy() const { return proxy_; }
  CoreType getCore() const { return default_core_; }
  NodeType getNode() const { return default_node_; }

  void mapToCore(CoreType const& core) { default_core_ = core; }
  void setCoreMap(HandlerType const han) { core_map_handle_ = han; }
  void setNodeMap(HandlerType const han) { node_map_handle_ = han; }
  bool hasCoreMap() const { return core_map_handle_ != uninitialized_handler; }
  bool hasNodeMap() const { return node_map_handle_ != uninitialized_handler; }
  void setSeed(SeedType const seed) { seed_ = seed; }

 private:
  HandlerType core_map_handle_ = uninitialized_handler;
  HandlerType node_map_handle_ = uninitialized_handler;

  CoreType default_core_ = uninitialized_destination;
  NodeType default_node_ = uninitialized_destination;
  VirtualProxyType proxy_ = no_vrt_proxy;
  AtomicType<bool> is_constructed_ = {false};
  VirtualPtrType vrt_ptr_ = nullptr;
  bool needs_lock_ = false;
  SeedType seed_ = no_seed;
  MsgBufferContainerType msg_buffer_;
};

}}  // end namespace vt::vrt

#endif /*INCLUDED_CONTEXT_VRT_INFO*/
