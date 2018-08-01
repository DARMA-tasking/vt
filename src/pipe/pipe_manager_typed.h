
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TYPED_H
#define INCLUDED_PIPE_PIPE_MANAGER_TYPED_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager_base.h"
#include "pipe/pipe_manager_typed.fwd.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/interface/callback_direct_multi.h"
#include "pipe/interface/callback_types.h"
#include "pipe/callback/handler_send/callback_send_han.h"
#include "pipe/pipe_manager_construct.h"

#include <type_traits>
#include <tuple>
#include <functional>

namespace vt { namespace pipe {

struct PipeManagerTyped : virtual PipeManagerBase {
  template <typename T>
  using CallbackTypes        = interface::CallbackTypes<T>;
  template <typename T>
  using CallbackSendType     = typename CallbackTypes<T>::CallbackDirectSend;
  template <typename T>
  using CallbackAnonType     = typename CallbackTypes<T>::CallbackAnon;
  template <typename T>
  using CallbackBcastType    = typename CallbackTypes<T>::CallbackDirectBcast;
  using CallbackAnonVoidType = typename CallbackTypes<void>::CallbackAnonVoid;

  template <typename MsgT>
  using FuncMsgType          = std::function<void(MsgT*)>;
  using FuncVoidType         = std::function<void(void)>;

  /*
   *  Builders for non-send-back type of pipe callback: they are invoked
   *  directly from the sender; thus this node is not involved in the process
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
  interface::CallbackDirectSendMulti<
    MsgT,
    typename RepeatNImpl<sizeof...(f),callback::CallbackSend<MsgT>>::ResultType
  >
  makeCallbackMultiSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTargetBcast(bool const& inc);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTargetBcast(CallbackT in, bool const& inc);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTarget(NodeType const& send_to_node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTarget(CallbackT in, NodeType const& send_to_node);

  template <typename CallbackT>
  auto buildMultiCB(CallbackT in);

  /*
   *  Make a fully typed single-endpoint listener callback that directly sends
   *  to the endpoint
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackSendType<MsgT> makeCallbackSingleSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  /*
   *  Make a fully typed single-endpoint anon function callback
   */
  template <typename MsgT>
  CallbackAnonType<MsgT> makeCallbackSingleAnonTyped(
    bool const persist, FuncMsgType<MsgT> fn
  );

  template <typename=void>
  CallbackAnonVoidType makeCallbackSingleAnonVoidTyped(
    bool const persist, FuncVoidType fn
  );

  /*
   *  Make a fully typed single-endpoint broadcast directly routed callback
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackBcastType<MsgT> makeCallbackSingleBcastTyped(bool const inc);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TYPED_H*/
