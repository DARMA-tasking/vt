/*
//@HEADER
// *****************************************************************************
//
//                                pipe_manager.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_PIPE_PIPE_MANAGER_H
#define INCLUDED_VT_PIPE_PIPE_MANAGER_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/pipe/pipe_manager_tl.h"
#include "vt/pipe/pipe_manager_typed.h"
#include "vt/pipe/msg/callback.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/callback/anon/callback_anon.fwd.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/pipe/pipe_lifetime.h"
#include "vt/activefn/activefn.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/proxy/proxy_objgroup_elm.h"
#include "vt/runtime/component/component_pack.h"

#include <unordered_map>
#include <functional>

namespace vt { namespace pipe {

/**
 * \struct PipeManager
 *
 * \brief Core VT component that provides an interface to create type-erased
 * callbacks to many endpoint types.
 *
 * Allows creation of callback to many types of handlers with different
 * modes---like send and broadcast.
 */
struct PipeManager
  : runtime::component::Component<PipeManager>,
    PipeManagerTL, PipeManagerTyped
{
  template <typename FunctorT>
  using GetMsgType = typename util::FunctorExtractor<FunctorT>::MessageType;
  using Void = V;

  /**
   * \internal \brief System constructor for making a new PipeManager component
   */
  PipeManager();

  std::string name() override { return "PipeManager"; }

  /**
   * \brief Make callback to an active function, objgroup, or collection
   *
   * \param[in] target the node or proxy to target
   *
   * \return a callback
   */
  template <auto f, typename Target>
  auto makeSend(Target target);

  /**
   * \brief Make callback to a functor
   *
   * \param[in] node the node to target
   *
   * \return a callback
   */
  template <typename FunctorT>
  auto makeSend(NodeType node);

  /**
   * \brief Make callback to an active function
   *
   * \return a callback
   */
  template <auto f>
  auto makeBcast();

  /**
   * \brief Make callback to a functor
   *
   * \return a callback
   */
  template <typename FunctorT>
  auto makeBcast();

  /**
   * \brief Make callback to an objgroup or collection
   *
   * \param[in] proxy the proxy to target
   *
   * \return a callback
   */
  template <auto f, typename ProxyT>
  auto makeBcast(ProxyT proxy);

  /**
   * \brief Make callback to a function (including lambdas) with a context
   * pointer to any object on this node.
   *
   * \warning One must ensure that the lifetime of context pointer provided to
   * the callback persists at least as long as the last time the callback might
   * be invoked.
   *
   * Example snippet:
   *
   * \code{.cpp}
   *  struct DataMsg : vt::Message { };
   *  struct Context { int val = 129; };
   *  int main() {
   *    auto ctx = std::make_unique<Context>();
   *    auto cb = vt::theCB()->makeFunc<DataMsg,Context>(
   *      ctx.get(), [](DataMsg* msg, Context* my_ctx){
   *        // callback triggered with message and associated context
   *      }
   *    );
   *    cb.send();
   *  }
   * \endcode
   *
   * \param[in] life the lifetime for this callback
   * \param[in] ctx pointer to the object context passed to callback function
   * \param[in] fn endpoint function that takes a message and context pointer
   *
   * \return a new callback
   */
  template <typename MsgT, typename ContextT>
  Callback<MsgT> makeFunc(
    LifetimeEnum life, ContextT* ctx, FuncMsgCtxType<MsgT, ContextT> fn
  );

  /**
   * \brief Make a void callback to a function (including lambdas) with a
   * context pointer to any object on this node.
   *
   * \warning One must ensure that the lifetime of context pointer provided to
   * the callback persists at least as long as the last time the callback might
   * be invoked.
   *
   * Example snippet:
   *
   * \code{.cpp}
   *  struct Context { int val = 129; };
   *  int main() {
   *    auto ctx = std::make_unique<Context>();
   *    auto cb = vt::theCB()->makeFunc<Context>(
   *      ctx.get(), [](Context* my_ctx){
   *        // callback triggered with associated context
   *      }
   *    );
   *    cb.send();
   *  }
   * \endcode
   *
   * \param[in] life the lifetime for this callback
   * \param[in] ctx pointer to the object context passed to callback function
   * \param[in] fn endpoint function that takes a context pointer
   *
   * \return a new callback
   */
  template <typename ContextT>
  Callback<> makeFunc(
    LifetimeEnum life, ContextT* ctx, FuncCtxType<ContextT> fn
  );

  /**
   * \brief Make a callback to a function (including lambdas) on this node with
   * a message.
   *
   * Example snippet:
   *
   * \code{.cpp}
   *  struct DataMsg : vt::Message { };
   *  int main() {
   *    auto cb = vt::theCB()->makeFunc<DataMsg>(
   *      [](DataMsg* msg){
   *        // callback triggered with message
   *      }
   *    );
   *    cb.send();
   *  }
   * \endcode
   *
   * \param[in] life the lifetime for this callback
   * \param[in] fn endpoint function that takes a message
   *
   * \return the new callback
   */
  template <typename MsgT>
  Callback<MsgT> makeFunc(LifetimeEnum life, FuncMsgType<MsgT> fn);

  /**
   * \brief Make a void callback to a function (including lambdas) on this node.
   *
   * Example snippet:
   *
   * \code{.cpp}
   *  int main() {
   *    auto cb = vt::theCB()->makeFunc([]{
   *      // callback triggered with message
   *    });
   *    cb.send();
   *  }
   * \endcode
   *
   * \param[in] life the lifetime for this callback
   * \param[in] fn void endpoint function
   *
   * \return the new callback
   */
  Callback<> makeFunc(LifetimeEnum life, FuncVoidType fn);

  /**
   * \brief Make a callback to a active message handler to be invoked on a
   * certain node with a message.
   *
   * \param[in] node node to invoke callback on
   *
   * \return the new callback
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  [[deprecated("Replace with call to makeSend<auto f>")]]
  Callback<MsgT> makeSend(NodeType const& node);

  /**
   * \brief Make a callback to a particular collection element invoking a
   * non-intrusive collection active function handler.
   *
   * \param[in] proxy element proxy to collection
   *
   * \return the new callback
   */
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  [[deprecated("Replace with call to makeSend<auto f>")]]
  Callback<MsgT> makeSend(typename ColT::ProxyType proxy);

  /**
   * \brief Make a callback to a particular collection element invoking an
   * intrusive collection member handler.
   *
   * \param[in] proxy element proxy to collection
   *
   * \return the new callback
   */
  template <typename ColT, typename MsgT, ColMemType<ColT,MsgT> f>
  [[deprecated("Replace with call to makeSend<auto f>")]]
  Callback<MsgT> makeSend(typename ColT::ProxyType proxy);

  /**
   * \brief Make a callback to a particular objgroup element (node) invoking a
   * objgroup member handler.
   *
   * \param[in] proxy element proxy to objgroup
   *
   * \return the new callback
   */
  template <typename ObjT, typename MsgT, ObjMemType<ObjT,MsgT> f>
  [[deprecated("Replace with call to makeSend<auto f>")]]
  Callback<MsgT> makeSend(objgroup::proxy::ProxyElm<ObjT> proxy);

  /**
   * \brief Make a callback to a active message handler with a message to be
   * broadcast to all nodes.
   *
   * \return the new callback
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  [[deprecated("Replace with call to makeBcast<auto f>")]]
  Callback<MsgT> makeBcast();

  /**
   * \brief Make a callback to a whole collection invoking a non-intrusive
   * collection active function handler.
   *
   * \param[in] proxy proxy to collection
   *
   * \return the new callback
   */
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  [[deprecated("Replace with call to makeBcast<auto f>")]]
  Callback<MsgT> makeBcast(ColProxyType<ColT> proxy);

  /**
   * \brief Make a callback to a whole collection invoking an intrusive
   * collection member handler.
   *
   * \param[in] proxy proxy to collection
   *
   * \return the new callback
   */
  template <typename ColT, typename MsgT, ColMemType<ColT,MsgT> f>
  [[deprecated("Replace with call to makeBcast<auto f>")]]
  Callback<MsgT> makeBcast(ColProxyType<ColT> proxy);

  /**
   * \brief Make a callback to a whole objgroup invoking a objgroup member
   * handler.
   *
   * \param[in] proxy proxy to objgroup
   *
   * \return the new callback
   */
  template <typename ObjT, typename MsgT, ObjMemType<ObjT,MsgT> f>
  [[deprecated("Replace with call to makeBcast<auto f>")]]
  Callback<MsgT> makeBcast(objgroup::proxy::Proxy<ObjT> proxy);

public:
  /**
   * \internal \brief Trigger and send back on a pipe that is not locally
   * triggerable and thus requires communication if it is "sent" off-node.
   *
   * \param[in] pipe the pipe ID
   * \param[in] data the message to send
   */
  template <typename MsgT>
  void triggerSendBack(PipeType const& pipe, MsgT* data);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | group_id_;
  }

private:
  /// The group ID used to indicate that the message is being used as a pipe
  GroupType group_id_ = no_group;
};

}} /* end namespace vt::pipe */

#include "vt/pipe/pipe_manager_base.impl.h"
#include "vt/pipe/pipe_manager_tl.impl.h"
#include "vt/pipe/pipe_manager_typed.impl.h"
#include "vt/pipe/pipe_manager.impl.h"

#endif /*INCLUDED_VT_PIPE_PIPE_MANAGER_H*/
