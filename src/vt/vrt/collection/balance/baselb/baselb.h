/*
//@HEADER
// *****************************************************************************
//
//                                   baselb.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/elm/elm_comm.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/objgroup/headers.h"

#include <set>
#include <map>
#include <unordered_map>
#include <tuple>

namespace vt { namespace vrt { namespace collection {

namespace balance {
struct LoadModel;
}

namespace lb {

struct CommMsg;

struct BaseLB {
  using ObjIDType        = balance::ElementIDStruct;
  using ElementLoadType  = std::unordered_map<ObjIDType,TimeType>;
  using ElementCommType  = elm::CommMapType;
  using TransferDestType = std::tuple<ObjIDType,NodeType>;
  using TransferVecType  = std::vector<TransferDestType>;
  using TransferType     = std::map<NodeType, TransferVecType>;
  using LoadType         = double;
  using MigrationCountCB = std::function<void(int32_t)>;
  using QuantityType     = std::map<lb::StatisticQuantity, double>;
  using StatisticMapType = std::unordered_map<lb::Statistic, QuantityType>;
  using LoadSummary      = balance::LoadSummary;
  using ObjLoadListType  = std::vector<
    std::tuple<ObjIDType, LoadSummary, LoadSummary>
  >;
  using ObjDestinationListType = std::vector<std::tuple<ObjIDType, NodeType>>;

  explicit BaseLB(bool in_comm_aware = false)
    : comm_aware_(in_comm_aware),
      pending_reassignment_(std::make_shared<balance::Reassignment>())
  { }

  BaseLB(BaseLB const &) = delete;
  BaseLB(BaseLB &&) noexcept = default;
  BaseLB &operator=(BaseLB const &) = delete;
  BaseLB &operator=(BaseLB &&) noexcept = default;

  virtual ~BaseLB() = default;

  /**
   * This sets up and invokes the particular strategy implementations
   * through virtual methods `initParams` and `runLB`, and then
   * normalizes their output to a reassignment that can be evaluated
   * and applied
   *
   * This must be called collectively.
   *
   * \return A normalized reassignment
   */
  std::shared_ptr<const balance::Reassignment> startLB(
    PhaseType phase,
    objgroup::proxy::Proxy<BaseLB> proxy,
    balance::LoadModel *model,
    StatisticMapType const& in_stats,
    ElementCommType const& in_comm_lb_data,
    TimeType total_load
  );

  void importProcessorData(
    StatisticMapType const& in_stats, ElementCommType const& cm
  );

  static LoadType loadMilli(LoadType const& load);

  void notifyCurrentHostNodeOfObjectsDeparting(
    TransferMsg<ObjDestinationListType>* msg
  );
  void notifyNewHostNodeOfObjectsArriving(
    TransferMsg<ObjLoadListType>* msg
  );

  void applyMigrations(
    TransferVecType const& transfers, MigrationCountCB migration_count_callback
  );
  void migrationDone();
  void migrateObjectTo(ObjIDType const obj_id, NodeType const node);
  void transferSend(NodeType from, TransferVecType const& transfer);
  void transferMigrations(TransferMsg<TransferVecType>* msg);
  void finalize(int32_t global_count);

  virtual void inputParams(balance::ConfigEntry* config) = 0;
  virtual void runLB(TimeType total_load) = 0;

  StatisticMapType const* getStats() const {
    return base_stats_;
  }

  TransferVecType& getTransfers() { return transfers_; }

  bool isCommAware() const { return comm_aware_; }
  void recvSharedEdges(CommMsg* msg);

protected:
  void getArgs(PhaseType phase);

  double start_time_                                  = 0.0f;
  ElementCommType const* comm_data                    = nullptr;
  objgroup::proxy::Proxy<BaseLB> proxy_               = {};
  PhaseType phase_                                    = 0;
  std::unique_ptr<balance::ConfigEntry> config_entry_ = nullptr;
  // Observer only - LBManager owns the instance
  balance::LoadModel* load_model_                     = nullptr;
  bool comm_aware_                                    = false;

  /**
   * \brief Normalizes the reassignment graph by setting up in/out edges on both
   * sides regardless of how they are passed to \c migrateObjectTo
   *
   * \return A normalized reassignment
   */
  std::shared_ptr<const balance::Reassignment> normalizeReassignments();

  static void setStrategySpecificModel(
    std::shared_ptr<balance::LoadModel> model
  );

private:
  TransferVecType transfers_                      = {};
  TransferType off_node_migrate_                  = {};
  int32_t local_migration_count_                  = 0;
  MigrationCountCB migration_count_cb_            = nullptr;
  StatisticMapType const* base_stats_             = nullptr;
  std::shared_ptr<balance::Reassignment> pending_reassignment_ = nullptr;
};

}}}} // namespace vt::vrt::collection::lb

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_H*/
