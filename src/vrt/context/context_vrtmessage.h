
#if !defined INCLUDED_CONTEXT_VRT_MESSAGE
#define INCLUDED_CONTEXT_VRT_MESSAGE

#include "config.h"
#include "messaging/message.h"
#include "topos/location/location_msg.h"

#include <cassert>
#include <type_traits>

namespace vt { namespace vrt {

template <typename MessageT>
using RoutedMessageType = LocationRoutedMsg<VirtualProxyType, MessageT>;

struct VirtualMessage : RoutedMessageType<vt::Message> {
  // By default, the `VirtualMessage' is byte copyable for serialization, but
  // derived classes may not be
  using isByteCopyable = std::true_type;

  VirtualMessage() = default;

  // The variable `vc_sub_handler_' manages the intended user handler the
  // VirtualMessage should trigger
  void setVrtHandler(HandlerType const& in_handler) {
    vt_sub_handler_ = in_handler;
  }
  HandlerType getVrtHandler() const {
    assert(vt_sub_handler_ != uninitialized_handler && "Must have a valid handler");
    return vt_sub_handler_;
  }

  // The variable `to_proxy_' manages the intended target of the VirtualMessage
  VirtualProxyType getProxy() const {
    assert(to_proxy_ != no_vrt_proxy && "Must have a valid proxy target");
    return to_proxy_;
  }
  void setProxy(VirtualProxyType const& in_proxy) { to_proxy_ = in_proxy; }

  // Force the message to always execute on the communication thread regardless
  // of the mapping of the virtual context to a core. Used for system messages.
  bool getExecuteCommThread() const { return execute_comm_thd_; }
  void setExecuteCommThread(bool const comm) { execute_comm_thd_ = comm; }

  // Explicitly write a serializer so derived user messages can contain non-byte
  // serialization
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    RoutedMessageType<vt::Message>::serialize(s);
    s | vt_sub_handler_;
    s | to_proxy_;
    s | execute_comm_thd_;
  }

private:
  bool execute_comm_thd_ = false;
  VirtualProxyType to_proxy_ = no_vrt_proxy;
  HandlerType vt_sub_handler_ = uninitialized_handler;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_MESSAGE*/
