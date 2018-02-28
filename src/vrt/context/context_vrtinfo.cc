
#include "config.h"
#include "vrt/context/context_vrtinfo.h"
#include "vrt/context/context_vrtmessage.h"
#include "utils/mutex/mutex.h"
#include "utils/atomic/atomic.h"
#include "registry/auto/vc/auto_registry_vc.h"
#include "registry/auto/map/auto_registry_map.h"
#include "worker/worker_headers.h"
#include "messaging/message.h"

#include <memory>
#include <cassert>
#include <atomic>

namespace vt { namespace vrt {

VirtualInfo::VirtualInfo(
  VirtualPtrType in_vrt_ptr, VirtualProxyType const& proxy_in, bool needs_lock
) : proxy_(proxy_in),
    is_constructed_(in_vrt_ptr != nullptr),
    vrt_ptr_(is_constructed_ ? std::move(in_vrt_ptr) : nullptr),
    needs_lock_(needs_lock)
{ }

void VirtualInfo::setVirtualContextPtr(VirtualPtrType in_vrt_ptr) {
  assert(in_vrt_ptr != nullptr && "Must have a valid vrt ptr");

  vrt_ptr_ = std::move(in_vrt_ptr);
  is_constructed_ = true;

  VirtualContext* ptr = vrt_ptr_.get();

  debug_print(
    vrt, node,
    "setVirtualContextPtr: set ptr=%p, attaching process fn\n",
    in_vrt_ptr.get()
  );

  msg_buffer_.attach([this,ptr](VirtualMessage* msg){
    theWorkerGrp()->enqueueCommThread([=]{
      enqueueWorkUnit(msg);
    });
  });
}

bool VirtualInfo::enqueueWorkUnit(VirtualMessage* msg) {
  using auto_registry::AutoActiveVCType;

  auto const sub_handler = msg->getVrtHandler();
  auto const vc_active_fn = auto_registry::getAutoHandlerVC(sub_handler);
  auto const vc_ptr = vrt_ptr_.get();

  assert(vc_ptr != nullptr && "Must be valid pointer");

  auto work_unit = [=]{
    vc_active_fn(msg, vc_ptr);
    messageDeref(msg);
  };

  bool const has_workers = theContext()->hasWorkers();
  bool const execute_comm = msg->getExecuteCommThread();

  if (has_workers) {
    if (hasCoreMap() && !execute_comm) {
      auto const core = getCore();
      theWorkerGrp()->enqueueForWorker(core, work_unit);
    } else {
      theWorkerGrp()->enqueueCommThread(work_unit);
    }
    return true;
  } else {
    work_unit();
    return false;
  }
}

void VirtualInfo::tryEnqueueWorkUnit(VirtualMessage* msg) {
  bool const is_constructed = is_constructed_.load();
  bool enqueued = true;

  debug_print(
    vrt, node,
    "tryEnqueueWorkUnit is_cons=%s\n", print_bool(is_constructed)
  );

  if (is_constructed) {
    enqueued = enqueueWorkUnit(msg);
  } else {
    msg_buffer_.push(msg);
  }

  if (enqueued) {
    messageRef(msg);
  }
}

VirtualContext* VirtualInfo::get() const {
  assert(vrt_ptr_ != nullptr and "Must have a valid context");
  return vrt_ptr_.get();
}

}}  // end namespace vt::vrt
