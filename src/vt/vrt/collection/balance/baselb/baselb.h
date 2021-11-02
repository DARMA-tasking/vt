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
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/objgroup/headers.h"
#include "vt/vrt/collection/balance/model/load_model.h"

#include <set>
#include <map>
#include <unordered_map>
#include <tuple>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct BaseLB {
  using ObjIDType        = balance::ElementIDStruct;
  using ElementLoadType  = std::unordered_map<ObjIDType,TimeType>;
  using ElementCommType  = balance::CommMapType;
  using TransferDestType = std::tuple<ObjIDType,NodeType>;
  using TransferVecType  = std::vector<TransferDestType>;
  using TransferType     = std::map<NodeType, TransferVecType>;
  using LoadType         = double;
  using MigrationCountCB = std::function<void(int32_t)>;
  using QuantityType     = std::map<lb::StatisticQuantity, double>;
  using StatisticMapType = std::unordered_map<lb::Statistic, QuantityType>;
  using LoadSummary      = balance::LoadSummary;
  using DepartListType   = std::vector<std::tuple<ObjIDType, LoadSummary>>;
  using ArriveListType   = std::vector<std::tuple<ObjIDType, NodeType>>;

  explicit BaseLB(
    bool in_comm_aware = false,
    bool in_comm_collectives = false,
    int32_t in_bin_size = default_bin_size
  ) : bin_size_(in_bin_size),
      comm_aware_(in_comm_aware),
      comm_collectives_(in_comm_collectives),
      pending_reassignment_(std::make_unique<Reassignment>())
  { }

  BaseLB(BaseLB const &) = delete;
  BaseLB(BaseLB &&) noexcept = default;
  BaseLB &operator=(BaseLB const &) = delete;
  BaseLB &operator=(BaseLB &&) noexcept = default;

  virtual ~BaseLB() = default;

  /**
   * This must invoke the particular strategy implementations through
   * virtual methods `initParams` and `runLB`
   *
   * This expects to be run within a collective epoch. When that epoch
   * is complete, the concrete strategy implementation should have
   * recorded a complete set of intended migrations in `transfers_`
   * through calls to `migrateObjectTo`. Callers can then access that
   * set using `getTransfers` and apply it using `applyMigrations`.
   */
  void startLB(
    PhaseType phase,
    objgroup::proxy::Proxy<BaseLB> proxy,
    balance::LoadModel *model,
    StatisticMapType const& in_stats,
    ElementCommType const& in_comm_stats,
    TimeType total_load
  );

  void importProcessorData(
    StatisticMapType const& in_stats, ElementCommType const& cm
  );

  static LoadType loadMilli(LoadType const& load);
  NodeType objGetNode(ObjIDType const id) const;

  /**
   * \brief Normalizes the reassignment graph by setting up in/out edges on both
   * sides regardless of how they are pass to \c migrateObjectTo
   *
   * \return A normalized reassignment
   */
  std::unique_ptr<balance::Reassignment> normalizeReassignments();
  void notifyDeparting(TransferMsg<DepartListType>* msg);
  void notifyArriving(TransferMsg<ArriveListType>* msg);
  void arriveLoadSummary(TransferMsg<DepartListType>* msg);

  void applyMigrations(
    TransferVecType const& transfers, MigrationCountCB migration_count_callback
  );
  void migrationDone();
  void migrateObjectTo(ObjIDType const obj_id, NodeType const node);
  void transferSend(NodeType from, TransferVecType const& transfer);
  void transferMigrations(TransferMsg<TransferVecType>* msg);
  void finalize(CountMsg* msg);

  virtual void inputParams(balance::SpecEntry* spec) = 0;
  virtual void runLB(TimeType total_load) = 0;

  StatisticMapType const* getStats() const {
    return base_stats_;
  }

  TransferVecType& getTransfers() { return transfers_; }

protected:
  void getArgs(PhaseType phase);

protected:
  double start_time_                              = 0.0f;
  ElementCommType const* comm_data                = nullptr;
  objgroup::proxy::Proxy<BaseLB> proxy_           = {};
  PhaseType phase_                                = 0;
  std::unique_ptr<balance::SpecEntry> spec_entry_ = nullptr;
  // Observer only - LBManager owns the instance
  balance::LoadModel* load_model_                 = nullptr;

private:
  TransferVecType transfers_                      = {};
  TransferType off_node_migrate_                  = {};
  int32_t local_migration_count_                  = 0;
  MigrationCountCB migration_count_cb_            = nullptr;
  StatisticMapType const* base_stats_             = nullptr;
  std::unique_ptr<balance::Reassignment> pending_reassignment_ = nullptr;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_H*/
