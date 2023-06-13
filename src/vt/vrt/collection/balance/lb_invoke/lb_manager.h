/*
//@HEADER
// *****************************************************************************
//
//                                 lb_manager.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_LB_MANAGER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_LB_MANAGER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke_msg.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/lb_invoke/phase_info.h"
#include "vt/utils/json/base_appender.h"

#include <functional>
#include <map>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct LoadModel;
struct LoadData;

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
  using LBProxyType      = objgroup::proxy::Proxy<lb::BaseLB>;
  using QuantityType     = lb::StatisticQuantityMap;
  using StatisticMapType = lb::StatisticMap;

  friend lb::BaseLB;

  /**
   * \internal \brief System call to construct a \c LBManager
   */
  LBManager() = default;
  LBManager(LBManager const&) = delete;
  LBManager(LBManager&&) = default;
  virtual ~LBManager();

  std::string name() override { return "LBManager"; }

  void startup() override;
  void finalize() override;
  void fatalError() override;

  static std::unique_ptr<LBManager> construct();

public:
  /**
   * \internal
   * \brief Decide which LB to invoke given a certain phase
   *
   * \param[in] phase the phase in question
   * \param[in] try_file whether to try to read from file
   *
   * \return the LB to run
   */
  LBType decideLBToRun(PhaseType phase, bool try_file = true);

  /**
   * \internal
   * \brief Get the proxy for the LBManager
   *
   * \return proxy to the \c LBManager
   */
  objgroup::proxy::Proxy<LBManager> getProxy() const {
    return proxy_;
  }

  /**
   * \internal
   * \brief Setup the proxy for the LBManager
   *
   * \param[in] proxy the proxy to set
   */
  void setProxy(objgroup::proxy::Proxy<LBManager> proxy) {
    proxy_ = proxy;
  }

  /**
   * \internal
   * \brief Collectively start load balancing after deciding which to run
   *
   * \param[in] phase the phase
   * \param[in] cb the callback for delivering the reassignment
   */
  void selectStartLB(PhaseType phase, vt::Callback<ReassignmentMsg> cb);

  /**
   * \internal
   * \brief Collectively start load balancing after deciding which to run
   *
   * \param[in] phase the phase
   */
  void selectStartLB(PhaseType phase);

  /**
   * \internal
   * \brief Collectively start load balancing
   *
   * \param[in] phase the phase
   * \param[in] lb the load balancer to run
   * \param[in] cb the callback for delivering the reassignment
   */
  void startLB(PhaseType phase, LBType lb, vt::Callback<ReassignmentMsg> cb);

  /**
   * \internal
   * \brief Print documentation for LB args for the chosen LB
   *
   * \param[in] lb the load balancer to query
   */
  static void printLBArgsHelp(LBType lb);

  /**
   * \internal
   * \brief Print documentation for LB args for all LBs or the one specified
   *
   * \param[in] lb the load balancer in use
   */
  static void printLBArgsHelp(std::string lb);

  void destroyLB();

protected:
  /**
   * \internal
   * \brief Call when LB is finished to complete post-LB actions
   *
   * \param[in] phase the phase
   */
  void finishedLB(PhaseType phase);

public:
  /**
   * \brief Set a model of expected object loads to use in place of
   * the current installed model
   *
   * \param[in] model the model to apply
   *
   * This should be called with a similarly-constructed model instance
   * on every node
   */
  void setLoadModel(std::shared_ptr<LoadModel> model);

  /**
   * \brief Get the system-set basic model of object load
   */
  std::shared_ptr<LoadModel> getBaseLoadModel() { return base_model_; }
  /**
   * \brief Get the currently installed model of object load
   */
  std::shared_ptr<LoadModel> getLoadModel() { return model_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | cached_phase_
      | cached_lb_
      | destroy_lb_
      | proxy_
      | base_model_
      | model_
      | lb_instances_
      | stats
      | created_lbstats_dir_;
  }

  void stagePreLBStatistics(const StatisticMapType &statistics);
  void stagePostLBStatistics(
    const StatisticMapType &statistics, int32_t migration_count
  );
  void commitPhaseStatistics(PhaseType phase);

  /**
   * \internal \brief Collectively construct a new load balancer
   *
   * \param[in] LB the type of strategy to instantiate
   * \param[in] lb_name optional name of the load balancer
   *
   * \return objgroup proxy to the new load balancer
   */
  template <typename LB>
  LBProxyType makeLB(std::string const& lb_name = {});

protected:
  /**
   * \internal
   * \brief Run the currently chosen load balancer
   *
   * \param[in] phase the phase
   * \param[in] cb the callback for delivering the reassignment
   */
  void runLB(PhaseType phase, vt::Callback<ReassignmentMsg> cb);

  void defaultPostLBWork(ReassignmentMsg* r);

public:
  /**
   * \brief Compute statistics given a load model
   *
   * \param[in] model the load model
   * \param[in] comm_collectives whether to consider collective communication
   * \param[in] phase the phase
   * \param[in] cb the callback to receive the statistics
   */
  void computeStatistics(
    std::shared_ptr<LoadModel> model, bool comm_collectives, PhaseType phase,
    vt::Callback<std::vector<balance::LoadData>> cb
  );

  void statsHandler(std::vector<balance::LoadData> const& in_stat_vec);

private:
  bool isCollectiveComm(elm::CommCategory cat) const;

  /**
   * \internal \brief Create the statistics file
   */
  void createStatisticsFile();

  /**
   * \internal \brief Close the statistics file
   */
  void closeStatisticsFile();

  void setStrategySpecificModel(std::shared_ptr<LoadModel> model) {
    strategy_specific_model_ = model;
  }

  PhaseType cached_phase_                  = no_lb_phase;
  LBType cached_lb_                        = LBType::NoLB;
  std::function<void()> destroy_lb_        = nullptr;
  objgroup::proxy::Proxy<LBManager> proxy_;
  std::shared_ptr<LoadModel> base_model_;
  std::shared_ptr<LoadModel> model_;
  std::shared_ptr<LoadModel> strategy_specific_model_;
  std::unordered_map<std::string, LBProxyType> lb_instances_;
  StatisticMapType stats;
  TimeType total_load_from_model = 0.;
  std::unique_ptr<lb::PhaseInfo> last_phase_info_ = nullptr;
  bool before_lb_stats_ = true;
  /// The appender for outputting statistics in JSON format
  std::unique_ptr<util::json::BaseAppender> statistics_writer_ = nullptr;
  /// Whether the LB statistics directory has been created
  bool created_lbstats_dir_ = false;
};

void makeGraphSymmetric(
  PhaseType phase, objgroup::proxy::Proxy<lb::BaseLB> proxy
);

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::LBManager* theLBManager();

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_LB_MANAGER_H*/
