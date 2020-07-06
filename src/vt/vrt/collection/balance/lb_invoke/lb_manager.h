/*
//@HEADER
// *****************************************************************************
//
//                                   lb_manager.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke_msg.h"
#include "vt/vrt/collection/balance/lb_invoke/start_lb_msg.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <functional>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct LBManager
 *
 * \brief A VT component that manages creation and coordination of load
 * balancers.
 *
 * Manages starting and stopping load balancers depending on user's selection
 * and invocation.
 */
struct LBManager : runtime::component::Component<LBManager> {
  using ArgType        = vt::arguments::ArgConfig;
  using ListenerFnType = std::function<void(PhaseType)>;

  /**
   * \internal \brief System call to construct a \c LBManager
   */
  LBManager() = default;
  LBManager(LBManager const&) = delete;
  LBManager(LBManager&&) = default;
  virtual ~LBManager() {}

  std::string name() override { return "LBManager"; }

  static std::unique_ptr<LBManager> construct();

public:
  /**
   * \internal \brief Decide which LB to invoke given a certain phase
   *
   * \param[in] phase the phase in question
   * \param[in] try_file whether to try to read from file
   *
   * \return the LB to run
   */
  LBType decideLBToRun(PhaseType phase, bool try_file = true);

  /**
   * \internal \brief Collectively wait for LB, used to invoke without
   * consideration of readiness of the state of live collections
   */
  void waitLBCollective();

  /**
   * \internal \brief Get the proxy for the LBManager
   *
   * \return proxy to the \c LBManager
   */
  objgroup::proxy::Proxy<LBManager> getProxy() const {
    return proxy_;
  }

  /**
   * \internal \brief Setup the proxy for the LBManager
   *
   * \param[in] proxy the proxy to set
   */
  void setProxy(objgroup::proxy::Proxy<LBManager> proxy) {
    proxy_ = proxy;
  }

  /**
   * \internal \brief Tell the manager the LB is finished.
   *
   * \warning This should *not* be called by the
   * user, only by load balancers. Not private/protected as friending every LBs
   * adds too much overhead

   * \param[in] phase the phase that is finished
   */
  void finishedRunningLB(PhaseType phase);

protected:
  /**
   * \internal \brief Collectively start load balancing
   *
   * \param[in] phase the phase
   * \param[in] lb the load balancer to run
   * \param[in] manual whether it's manual or invoked from a collection
   * \param[in] num_calls number of calls required to start
   */
  void collectiveImpl(
    PhaseType phase, LBType lb, bool manual, std::size_t num_calls = 1
  );

  /**
   * \internal \brief Release control back to user
   *
   * \param[in] phase the phase
   * \param[in] num_calls number of calls required to start
   */
  void releaseImpl(PhaseType phase, std::size_t num_calls = 0);

  /**
   * \internal \brief Release control back to user now (without counting down)
   *
   * \param[in] phase the phase
   */
  void releaseNow(PhaseType phase);

public:
  /**
   * \internal \brief Print the memory usage for a phase
   *
   * \param[in] phase the phase
   */
  void printMemoryUsage(PhaseType phase);

  /**
   * \internal \brief Communicate to the trace component that a new phase
   * occurred, so tracing can be enabled or disabled
   *
   * \param[in] phase the phase
   */
  void setTraceEnabledNextPhase(PhaseType phase);

  /**
   * \internal \brief Communicate to the trace component that a new phase
   * occurred so flushing of traces can occur if required
   */
  void flushTraceNextPhase();

  /**
   * \internal \brief Tell the manager that a collection has hit \c nextPhase so
   * load balancing can begin once all collections enter
   *
   * \param[in] msg the LB message
   */
  template <typename MsgT>
  void sysLB(MsgT* msg) {
    vt_debug_print(lb, node, "sysLB\n");
    printMemoryUsage(msg->phase_);
    flushTraceNextPhase();
    setTraceEnabledNextPhase(msg->phase_);
    return collectiveImpl(msg->phase_, msg->lb_, msg->manual_, msg->num_collections_);
  }

  /**
   * \internal \brief Tell the manager that a collection has hit \c nextPhase,
   * choosing to skip load balancing
   *
   * \param[in] msg the LB message
   */
  template <typename MsgT>
  void sysReleaseLB(MsgT* msg) {
    vt_debug_print(lb, node, "sysReleaseLB\n");
    printMemoryUsage(msg->phase_);
    flushTraceNextPhase();
    setTraceEnabledNextPhase(msg->phase_);
    return releaseImpl(msg->phase_, msg->num_collections_);
  }

public:
  /**
   * \brief Register a listener to trigger after LB completes
   *
   * \param[in] fn the listener
   *
   * \return the ID of the registration
   */
  int registerListenerAfterLB(ListenerFnType fn);

  /**
   * \brief Unregister a listener to trigger after LB completes
   *
   * \param[in] element the registration ID
   */
  void unregisterListenerAfterLB(int element);

  /**
   * \internal \brief Trigger all after-LB listeners
   *
   * \param[in] phase the phase
   */
  void triggerListeners(PhaseType phase);

  /**
   * \brief Set a model of expected object loads to use in place of
   * naive persistence
   *
   */
  void setLoadModel(std::unique_ptr<LoadModel> model);

protected:
  /**
   * \internal \brief Collectively construct a new load balancer
   *
   * \param[in] msg the start LB message
   *
   * \return objgroup proxy to the new load balancer
   */
  template <typename LB>
  objgroup::proxy::Proxy<LB> makeLB(MsgSharedPtr<StartLBMsg> msg);

private:
  std::size_t num_invocations_             = 0;
  std::size_t num_release_                 = 0;
  PhaseType cached_phase_                  = no_lb_phase;
  LBType cached_lb_                        = LBType::NoLB;
  std::function<void()> destroy_lb_        = nullptr;
  bool synced_in_lb_                       = true;
  std::vector<ListenerFnType> listeners_   = {};
  objgroup::proxy::Proxy<LBManager> proxy_;
  std::unique_ptr<LoadModel> model_        = nullptr;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::LBManager* theLBManager();

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H*/
