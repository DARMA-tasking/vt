
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_H

#include "config.h"
#include "topos/location/message/msg.h"
#include "messaging/message.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/vrt_common.h"

#include <type_traits>

namespace vt { namespace vrt { namespace collection {

template <typename MessageT, typename ColT>
using RoutedMessageType = LocationRoutedMsg<
  ::vt::vrt::VirtualElmProxyType<ColT, typename ColT::IndexType>, MessageT
>;

static struct ColMsgWrapTagType { } ColMsgWrapTag { };

template <typename ColT, typename BaseMsgT = ::vt::Message>
struct CollectionMessage :
  RoutedMessageType<BaseMsgT, ColT>, ColT::IndexType::IsByteCopyable
{
  /*
   *. Type aliases for surrounding system => used to deduce during sends
  */
  using CollectionType = ColT;
  using IndexType = typename ColT::IndexType;
  using IsCollectionMessage = std::true_type;
  using UserMsgType = void;

  CollectionMessage() = default;

  explicit CollectionMessage(ColMsgWrapTagType)
    : is_wrap_(true)
  { }

  void setVrtHandler(HandlerType const& in_handler);
  HandlerType getVrtHandler() const;

  // The variable `to_proxy_' manages the intended target of the
  // `CollectionMessage'
  VirtualElmProxyType<ColT, IndexType> getProxy() const;
  void setProxy(VirtualElmProxyType<ColT, IndexType> const& in_proxy);

  VirtualProxyType getBcastProxy() const;
  void setBcastProxy(VirtualProxyType const& in_proxy);

  EpochType getBcastEpoch() const;
  void setBcastEpoch(EpochType const& epoch);

  NodeType getFromNode() const;
  void setFromNode(NodeType const& node);

  bool getMember() const;
  void setMember(bool const& member);

  bool getWrap() const;
  void setWrap(bool const& wrap);

  #if backend_check_enabled(lblite)
    bool lbLiteInstrument() const;
    void setLBLiteInstrument(bool const& val);
  #endif

  // Explicitly write a parent serializer so derived user messages can contain
  // non-byte serialization
  template <typename SerializerT>
  void serializeParent(SerializerT& s);

  template <typename SerializerT>
  void serializeThis(SerializerT& s);

  friend struct CollectionManager;

private:
  VirtualProxyType bcast_proxy_{};
  VirtualElmProxyType<ColT, IndexType> to_proxy_{};
  HandlerType vt_sub_handler_ = uninitialized_handler;
  EpochType bcast_epoch_ = no_epoch;
  NodeType from_node_ = uninitialized_destination;
  bool member_ = false;
  bool is_wrap_ = false;

  #if backend_check_enabled(lblite)
    /*
     * By default this is off so system messages do not all get
     * instrumented. When the user sends a message through theCollection
     * (sendMsg,broadcastMsg) they are automatically instrumented
     */
    bool lb_lite_instrument_ = false;
  #endif
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

template <typename ColT, typename MsgT = ::vt::Message>
using CollectionMessage = vrt::collection::CollectionMessage<ColT, MsgT>;

} /* end namespace vt */

#include "vrt/collection/messages/user.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_H*/
