/*
//@HEADER
// *****************************************************************************
//
//                                 temperedlb.h
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

#include "vt/configs/types/types_type.h"
#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/temperedlb/tempered_msgs.h"
#include "vt/vrt/collection/balance/temperedlb/criterion.h"
#include "vt/vrt/collection/balance/temperedlb/tempered_enums.h"

#include <random>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct TemperedLB : BaseLB {
  using LoadMsgAsync   = balance::LoadMsgAsync;
  using LoadMsgSync    = balance::LoadMsg;
  using NodeSetType    = std::vector<NodeType>;
  using ObjsType       = std::unordered_map<ObjIDType, LoadType>;
  using ReduceMsgType  = vt::collective::ReduceNoneMsg;
  using QuantityType     = std::map<lb::StatisticQuantity, double>;
  using StatisticMapType = std::unordered_map<lb::Statistic, QuantityType>;
  using EdgeMapType      = std::unordered_map<
    elm::ElementIDStruct, std::vector<std::tuple<elm::ElementIDStruct, double>>
  >;

  TemperedLB() = default;
  TemperedLB(TemperedLB const&) = delete;

  virtual ~TemperedLB() {}

public:
  void init(objgroup::proxy::Proxy<TemperedLB> in_proxy);
  void runLB(LoadType total_load) override;
  void inputParams(balance::ConfigEntry* config) override;

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

  static std::vector<ObjIDType> orderObjects(
    ObjectOrderEnum obj_ordering,
    std::unordered_map<ObjIDType, LoadType> cur_objs,
    LoadType this_new_load, LoadType target_max_load
  );

protected:
  void doLBStages(LoadType start_imb);
  void informAsync();
  void informSync();
  void originalTransfer();
  void swapClusters();
  void migrate();

  void propagateRound(uint8_t k_cur_async, bool sync, EpochType epoch = no_epoch);
  void propagateIncomingAsync(LoadMsgAsync* msg);
  void propagateIncomingSync(LoadMsgSync* msg);
  bool isUnderloaded(LoadType load) const;
  bool isUnderloadedRelaxed(LoadType over, LoadType under) const;
  bool isOverloaded(LoadType load) const;

  std::vector<double> createCMF(NodeSetType const& under);
  NodeType sampleFromCMF(NodeSetType const& under, std::vector<double> const& cmf);
  std::vector<NodeType> makeUnderloaded() const;
  std::vector<NodeType> makeSufficientlyUnderloaded(
    LoadType load_to_accommodate
  ) const;
  ElementLoadType::iterator selectObject(
    LoadType size, ElementLoadType& load, std::set<ObjIDType> const& available
  );
  virtual LoadType getModeledValue(const elm::ElementIDStruct& obj);

  void lazyMigrateObjsTo(EpochType epoch, NodeType node, ObjsType const& objs);
  void inLazyMigrations(balance::LazyMigrationMsg* msg);
  void loadStatsHandler(std::vector<balance::LoadData> const& vec);
  void workStatsHandler(std::vector<balance::LoadData> const& vec);
  void rejectionStatsHandler(int n_rejected, int n_transfers);
  void thunkMigrations();

  void setupDone();

  /**
   * \brief Read the memory data from the user-defined json blocks into data
   * structures
   */
  void readClustersMemoryData();

  /**
   * \brief Compute the memory usage for current assignment
   *
   * \return the total memory usage
   */
  BytesType computeMemoryUsage();

  /**
   * \brief Get the shared blocks that are located on this node with the current
   * object assignment
   *
   * \return the number of shared blocks here
   */
  std::set<SharedIDType> getSharedBlocksHere() const;

  /**
   * \brief Compute the current cluster assignment summary for this rank
   */
  void computeClusterSummary();

  /**
   * \brief Try to lock a rank
   *
   * \param[in] requesting_node the requesting rank asking to lock
   * \param[in] criterion_value the criterion evaluation value to compare
   */
  void tryLock(NodeType requesting_node, double criterion_value);

  /**
   * \struct LockedInfoMsg
   *
   * \brief The update message that comes from a rank when it is locked. This is
   * a message instead of a normal handler so it can be buffered without copying
   * it.
   */
  struct LockedInfoMsg : vt::Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required(); // locked_clusters_

    LockedInfoMsg() = default;
    LockedInfoMsg(
      NodeType in_locked_node, LoadType in_locked_load,
      ClusterSummaryType in_locked_clusters, BytesType in_locked_bytes,
      BytesType in_locked_max_object_working_bytes,
      double in_locked_c_try
    ) : locked_node(in_locked_node),
        locked_load(in_locked_load),
        locked_clusters(in_locked_clusters),
        locked_bytes(in_locked_bytes),
        locked_max_object_working_bytes(in_locked_max_object_working_bytes),
        locked_c_try(in_locked_c_try)
    { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | locked_node;
      s | locked_load;
      s | locked_clusters;
      s | locked_bytes;
      s | locked_max_object_working_bytes;
      s | locked_c_try;
    }

    /// The node that is locked
    NodeType locked_node = uninitialized_destination;
    /// The total load of the locked node
    LoadType locked_load = 0;
    /// The up-to-date summary of the clusters
    ClusterSummaryType locked_clusters = {};
    /// The total bytes for the locked node
    BytesType locked_bytes = 0;
    /// The largest working bytes for the locked node
    BytesType locked_max_object_working_bytes = 0;
    /// The approximate criterion value at the time it was locked with possible
    /// out-of-date info
    double locked_c_try = 0;
  };

  /**
   * \brief Satisfy a lock request (if there is one)
   */
  void satisfyLockRequest();

  /**
   * \brief Inform a rank that a lock was obtained
   *
   * \param[in] msg update message with all the info
   */
  void lockObtained(LockedInfoMsg* msg);

  /**
   * \brief Compute memory component of tempered transfer criterion
   *
   * \param[in] try_total_bytes: total memory bytes on target rank
   * \param[in] src_bytes: memory bytes to be transferred from source rank
   */
  bool memoryTransferCriterion(double try_total_bytes, double src_bytes);

  /**
   * \brief Compute load component of tempered transfer criterion
   *
   * \param[in] before_w_src: original work on source rank
   * \param[in] before_w_dst: original work on destination rank
   * \param[in] src_l: sum of object loads to be transferred from source
   * \param[in] dst_l: sum of object loads to be transferred from destination
   */
  double loadTransferCriterion(double before_w_src, double before_w_dst, double src_l, double dst_l);

  /**
   * \brief Compute the amount of work based on the work model
   *
   * \note Model: α * load + β * inter_comm_bytes + δ * intra_comm_bytes + γ
   *
   * \param[in] load the load for a rank
   * \param[in] comm_bytes the external communication
   *
   * \return the amount of work
   */
  double computeWork(
    double load, double inter_comm_bytes, double intra_comm_bytes
  ) const;

  /**
   * \brief Compute the rank's work
   *
   *  \param[in] exclude a set of objects to exclude that are in cur_objs_
   *  \param[in] include a set of objects to include that are not in cur_objs_
   *
   * \return the amount of work currently for the set of objects
   */
  double computeRankWork(
    std::set<ObjIDType> exclude = {},
    std::set<ObjIDType> include = {}
  );

  /**
   * \brief Consider possible swaps with all the up-to-date info from a rank
   *
   * \param[in] msg update message with all the info
   */
  void considerSwapsAfterLock(MsgSharedPtr<LockedInfoMsg> msg);

  /**
   * \brief Consider possible subcluster transfers with all the up-to-date info
   * from a rank
   *
   * \param[in] msg update message with all the info
   */
  void considerSubClustersAfterLock(MsgSharedPtr<LockedInfoMsg> msg);

  /**
   * \brief Release a lock on a rank
   */
  void releaseLock();

  /**
   * \brief Try sub-clustering---i.e., breaking up clusters to improve LB
   */
  void trySubClustering();

  /**
   * \brief Give a cluster to a rank
   *
   * \param[in] from_rank the rank it's coming from
   * \param[in] give_shared_blocks_size the shared block info for the swap
   * \param[in] give_objs the objects given
   * \param[in] give_obj_shared_block the shared block the objs are part of
   * \param[in] give_obj_working_bytes the working bytes for the objs
   * \param[in] take_cluster (optional) a cluster requested in return
   */
  void giveCluster(
    NodeType from_rank,
    std::unordered_map<SharedIDType, BytesType> const& give_shared_blocks_size,
    std::unordered_map<ObjIDType, LoadType> const& give_objs,
    std::unordered_map<ObjIDType, SharedIDType> const& give_obj_shared_block,
    std::unordered_map<ObjIDType, BytesType> const& give_obj_working_bytes,
    SharedIDType take_cluster
  );

  /**
   * \internal \brief Remove a cluster to send. Does all the bookkeeping
   * associated with removing the cluster
   *
   * \param[in] shared_id the shared ID of the cluster to remove
   * \param[in] objs the set of objects to send with that shared ID (optional,
   * if not specified then send all of them)
   *
   * \return a tuple with all the information to send to \c giveCluster
   */
  auto removeClusterToSend(SharedIDType shared_id, std::set<ObjIDType> objs = {});

private:
  uint16_t f_                                       = 0;
  uint8_t k_max_                                    = 0;
  uint8_t k_cur_                                    = 0;
  uint16_t iter_                                    = 0;
  uint16_t trial_                                   = 0;
  uint16_t num_iters_                               = 4;
  /**
   * \brief Number of trials
   *
   * How many times to repeat the requested number of iterations, hoping to find
   * a better imbalance. This helps if it's easy to get stuck in a local minimum.
   */
  uint16_t num_trials_                              = 1;
  /**
   * \brief Whether to make migration choices deterministic
   *
   * This will only lead to reproducibility when paired with deterministic
   * object loads, for example when using a driver that feeds the load balancer
   * object loads read from vt LB data files.
   */
  bool deterministic_                               = false;
  /**
   * \brief Whether to roll back to the best iteration
   *
   * If the final iteration of a trial has a worse imbalance than any earlier
   * iteration, it will roll back to the best iteration.
   */
  bool rollback_                                    = true;
  /**
   * \brief Whether to adjust the target load when we have a long pole
   *
   * When an object load exceeds the processor-average load (i.e., we have a
   * "long pole"), adjust the target load to be the maximum object load
   * ("longest pole") instead of the processor-average load.
   */
  bool target_pole_                                 = false;
  std::random_device seed_;
  std::unordered_map<NodeType, LoadType> load_info_ = {};
  std::unordered_map<NodeType, LoadType> new_load_info_ = {};
  objgroup::proxy::Proxy<TemperedLB> proxy_         = {};
  bool is_overloaded_                               = false;
  bool is_underloaded_                              = false;
  std::unordered_set<NodeType> selected_            = {};
  std::unordered_set<NodeType> underloaded_         = {};
  std::unordered_set<NodeType> new_underloaded_     = {};
  std::unordered_map<ObjIDType, LoadType> cur_objs_ = {};
  EdgeMapType send_edges_;
  EdgeMapType recv_edges_;
  LoadType this_new_load_                           = 0.0;
  LoadType this_new_work_                           = 0.0;
  LoadType new_imbalance_                           = 0.0;
  LoadType new_work_imbalance_                      = 0.0;
  LoadType work_mean_                               = 0.0;
  LoadType work_max_                                = 0.0;
  LoadType target_max_load_                         = 0.0;
  CriterionEnum criterion_                          = CriterionEnum::ModifiedGrapevine;
  InformTypeEnum inform_type_                       = InformTypeEnum::AsyncInform;
  /// Type of strategy to be used in transfer stage
  TransferTypeEnum transfer_type_                   = TransferTypeEnum::Original;
  ObjectOrderEnum obj_ordering_                     = ObjectOrderEnum::FewestMigrations;
  CMFTypeEnum cmf_type_                             = CMFTypeEnum::NormByMax;
  KnowledgeEnum knowledge_                          = KnowledgeEnum::Log;
  bool setup_done_                                  = false;
  bool propagate_next_round_                        = false;
  double α = 1.0;
  double β = 0.0;
  double γ = 0.0;
  double δ = 0.0;
  std::vector<bool> propagated_k_;
  std::mt19937 gen_propagate_;
  std::mt19937 gen_sample_;
  StatisticMapType stats;
  LoadType this_load                                = 0.0f;
  LoadType this_work                                = 0.0f;
  /// Whether any node has communication data
  bool has_comm_any_ = false;

  void hasCommAny(bool has_comm_any);
  void giveEdges(EdgeMapType const& edge_map);

  //////////////////////////////////////////////////////////////////////////////
  // All the memory info (may or may not be present)
  //////////////////////////////////////////////////////////////////////////////

  struct TryLock {
    TryLock(NodeType in_requesting, double in_c_try)
      : requesting_node(in_requesting),
        c_try(in_c_try)
    { }

    NodeType requesting_node = uninitialized_destination;
    double c_try = 0;

    double operator<(TryLock const& other) const {
      // sort in reverse order so the best is first!
      return c_try > other.c_try;
    }
  };

  struct ObjLoad {
    ObjLoad(ObjIDType in_obj_id, LoadType in_load)
      : obj_id(in_obj_id),
        load(in_load)
    { }

    ObjIDType obj_id = {};
    LoadType load = 0;

    double operator<(ObjLoad const& other) const {
      return load < other.load;
    }
  };

  /// Whether we have memory information
  bool has_memory_data_ = false;
  /// Working bytes for this rank
  BytesType rank_bytes_ = 0;
  /// Shared ID for each object
  std::unordered_map<ObjIDType, SharedIDType> obj_shared_block_;
  /// Shared block size in bytes
  std::unordered_map<SharedIDType, BytesType> shared_block_size_;
  /// Working bytes for each object
  std::unordered_map<ObjIDType, BytesType> obj_working_bytes_;
  /// Cluster summary based on current local assignment
  ClusterSummaryType cur_clusters_;
  /// Clusters that we know of on other ranks (might be out of date)
  std::unordered_map<NodeType, ClusterSummaryType> other_rank_clusters_;
  /// Working bytes for ranks we know about (never out of date)
  std::unordered_map<NodeType, BytesType> other_rank_working_bytes_;
  /// User-defined memory threshold
  BytesType mem_thresh_ = 0;
  /// The max working bytes for an object currently residing here
  BytesType max_object_working_bytes_ = 0;
  /// Current memory usage based on distribution
  BytesType current_memory_usage_ = 0;
  /// Whether this rank is locked or now
  bool is_locked_ = false;
  // Which rank locked this rank
  NodeType locking_rank_ = uninitialized_destination;
  /// Try locks that have arrived from other ranks
  std::set<TryLock> try_locks_;
  /// Pending operations that are waiting for an unlock
  std::list<ActionType> pending_actions_;
  /// Number of swaps so far
  int n_transfers_swap_ = 0;
  /// Whether it's mid-swap or not
  bool is_swapping_ = false;
  /// Max-load over ranks vector
  std::vector<LoadType> max_load_over_iters_;
  /// Whether we are sub-clustering
  bool is_subclustering_ = false;
  /// Ready to satify looks
  bool ready_to_satisfy_locks_ = false;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H*/
