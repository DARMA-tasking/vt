
#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_H

#include "vt/config.h"
#include "vt/vrt/collection/traits/coll_msg.h"

#include <type_traits>

namespace vt { namespace vrt { namespace collection {

struct DispatchCollectionBase {
  template <typename T, typename U=void>
  using IsColMsgType = std::enable_if_t<ColMsgTraits<T>::is_coll_msg>;
  template <typename T, typename U=void>
  using IsNotColMsgType = std::enable_if_t<!ColMsgTraits<T>::is_coll_msg>;

  DispatchCollectionBase() = default;

public:
  virtual void broadcast(
    VirtualProxyType proxy, void* msg, HandlerType han, bool member,
    ActionType action
  ) = 0;
  virtual void send(
    VirtualProxyType proxy, void* idx, void* msg, HandlerType han, bool member,
    ActionType action
  ) = 0;

  template <typename=void>
  VirtualProxyType getDefaultProxy() const;

  template <typename=void>
  void setDefaultProxy(VirtualProxyType const& in_proxy);

private:
  VirtualProxyType default_proxy_ = no_vrt_proxy;
};

template <typename ColT, typename MsgT>
struct DispatchCollection final : DispatchCollectionBase {
private:
  void broadcast(
    VirtualProxyType proxy, void* msg, HandlerType han, bool member,
    ActionType action
  ) override;
  void send(
    VirtualProxyType proxy, void* idx, void* msg, HandlerType han, bool member,
    ActionType action
  ) override;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_H*/
