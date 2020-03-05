/*
//@HEADER
// *****************************************************************************
//
//                                   invoke.h
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
#include "vt/configs/arguments/args.h"
#include "vt/objgroup/headers.h"

#include <functional>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct LBManager {
  using ArgType        = vt::arguments::ArgConfig;
  using ListenerFnType = std::function<void(PhaseType)>;

  LBManager() = default;
  LBManager(LBManager const&) = delete;
  LBManager(LBManager&&) = default;
  virtual ~LBManager() {}

  static void init() {
    LBManager::proxy_ = theObjGroup()->makeCollective<LBManager>();
  }

  static void destroy() {
    theObjGroup()->destroyCollective(LBManager::proxy_);
  }

public:
  /*
   * Decide which LB to invoke given a certain phase
   */
  LBType decideLBToRun(PhaseType phase, bool try_file = true);

  /*
   * Collectively wait for LB, used to invoke without consideration of readiness
   * of the state of live collections
   */
  void waitLBCollective();

  /*
   * Get the proxy for the LBManager
   */
  static objgroup::proxy::Proxy<LBManager> getProxy() { return proxy_; }

  /*
   * Tell the manage the LB is finished. This should *not* be called by the
   * user, only by load balancers. Not private/protected as friending every LBs
   * adds too much overhead
   */
  static void finishedRunningLB(PhaseType phase);

protected:
  void collectiveImpl(
    PhaseType phase, LBType lb, bool manual, std::size_t num_calls = 1
  );
  void releaseImpl(PhaseType phase, std::size_t num_calls = 0);
  void releaseNow(PhaseType phase);

public:
  void printMemoryUsage(PhaseType phase);
  void setTraceEnabledNextPhase(PhaseType phase);
  void flushTraceNextPhase();

  template <typename MsgT>
  void sysLB(MsgT* msg) {
    debug_print(lb, node, "sysLB\n");
    printMemoryUsage(msg->phase_);
    flushTraceNextPhase();
    setTraceEnabledNextPhase(msg->phase_);
    return collectiveImpl(msg->phase_, msg->lb_, msg->manual_, msg->num_collections_);
  }
  template <typename MsgT>
  void sysReleaseLB(MsgT* msg) {
    debug_print(lb, node, "sysReleaseLB\n");
    printMemoryUsage(msg->phase_);
    flushTraceNextPhase();
    setTraceEnabledNextPhase(msg->phase_);
    return releaseImpl(msg->phase_, msg->num_collections_);
  }

public:
  int registerListenerAfterLB(ListenerFnType fn);
  void unregisterListenerAfterLB(int element);
  void triggerListeners(PhaseType phase);

protected:
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

  static objgroup::proxy::Proxy<LBManager> proxy_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H*/
