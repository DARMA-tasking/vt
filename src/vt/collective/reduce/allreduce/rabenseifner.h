/*
//@HEADER
// *****************************************************************************
//
//                                rabenseifner.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/message/message.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/pipe/pipe_manager.h"
#include "data_handler.h"
#include "type.h"
#include "rabenseifner_msg.h"
#include "helpers.h"
#include "vt/utils/fntraits/fntraits.h"
#include "vt/configs/types/types_type.h"

#include <cstdint>

namespace vt::collective::reduce::allreduce {

struct CollectionAllreduceT {};
struct GroupAllreduceT {};
struct ObjgroupAllreduceT {};

/**
 * \struct Rabenseifner
 * \brief Class implementing Rabenseifner's allreduce algorithm.
 *
 * This class performs an allreduce operation using Rabenseifner's method. The algorithm consists
 * of several phases: adjustment for power-of-two processes, scatter-reduce, and gather-allgather.
 *
 * \tparam DataT Type of the data being reduced.
 * \tparam Op Reduction operation (e.g., sum, max, min).
 * \tparam finalHandler Callback handler for the final result.
 */
template <template <typename Arg> class Op>
struct Rabenseifner {
  // using Data = DataT;
  // using DataType = DataHandler<DataT>;
  // using Scalar = typename DataType::Scalar;
  // using ReduceOp = Op<Scalar>;
  // using DataHelperT = DataHelper<Scalar, DataT>;
  // using StateT = State<Scalar, DataT>;

  // using Trait = ObjFuncTraits<decltype(f)>;
  // using CallbackType =
  //   typename Trait::template WrapType<pipe::PipeManagerTL::CallbackRetType>;

  Rabenseifner(detail::StrongVrtProxy proxy, detail::StrongGroup group, size_t num_elems);
  Rabenseifner(detail::StrongGroup group);
  Rabenseifner(detail::StrongObjGroup objgroup);

  template <typename DataT, typename CallbackType>
  void setFinalHandler(const CallbackType& fin, size_t id);

  template <typename DataT, typename... Args>
  void localReduce(size_t id, Args&&... args);
  /**
   * \brief Initialize the allreduce algorithm.
   *
   * This function sets up the necessary data structures and initial values for the reduction operation.
   *
   * \param args Additional arguments for initializing the data value.
   */
  template <typename DataT, typename ...Args>
  void initialize(size_t id, Args&&... args);

  template <typename DataT>
  void initializeState(size_t id);
  size_t generateNewId() { return id_++; }

  /**
   * \brief Execute the final handler callback with the reduced result.
   */
  template <typename DataT>
  void executeFinalHan(size_t id);

  /**
   * \brief Perform the allreduce operation.
   *
   * This function starts the allreduce operation, adjusting for non-power-of-two process counts if necessary.
   */
  template <typename DataT>
  void allreduce(size_t id);

  /**
   * \brief Adjust the process count to the nearest power-of-two.
   *
   * This function performs additional steps to handle non-power-of-two process counts, ensuring that the
   * main scatter-reduce and gather-allgather phases can proceed with a power-of-two number of processes.
   */
  template <typename DataT>
  void adjustForPowerOfTwo(size_t id);

  /**
   * \brief Handler for adjusting the right half of the process group.
   *
   * This function handles the data received from the partner process and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  template <typename DataT, typename Scalar = typename DataHandler<DataT>::Scalar>
  void adjustForPowerOfTwoRightHalf(RabenseifnerMsg<Scalar, DataT>* msg);

  /**
   * \brief Handler for adjusting the left half of the process group.
   *
   * This function handles the data received from the partner process and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  template <typename DataT, typename Scalar = typename DataHandler<DataT>::Scalar>
  void adjustForPowerOfTwoLeftHalf(RabenseifnerMsg<Scalar, DataT>* msg);

  /**
   * \brief Final adjustment step for non-power-of-two process counts.
   *
   * This function handles the final step of the adjustment phase, combining the data and proceeding to the next phase.
   *
   * \param msg Message containing the data from the partner process.
   */
  template <typename DataT, typename Scalar = typename DataHandler<DataT>::Scalar>
  void adjustForPowerOfTwoFinalPart(RabenseifnerMsg<Scalar, DataT>* msg);

  /**
   * \brief Check if all scatter messages have been received.
   *
   * \return True if all scatter messages have been received, false otherwise.
   */
  template <typename DataT>
  bool scatterAllMessagesReceived(size_t id);

  /**
   * \brief Check if the scatter phase is complete.
   *
   * \return True if the scatter phase is complete, false otherwise.
   */
  template <typename DataT>
  bool scatterIsDone(size_t id);

  /**
   * \brief Check if the scatter phase is ready to proceed.
   *
   * \return True if the scatter phase is ready to proceed, false otherwise.
   */
  template <typename DataT>
  bool scatterIsReady(size_t id);

  /**
   * \brief Try to reduce the received scatter messages.
   *
   * \param step The current step in the scatter phase.
   */
  template <typename DataT>
  void scatterTryReduce(size_t id, int32_t step);

  /**
   * \brief Perform the scatter-reduce iteration.
   *
   * This function sends data to the appropriate partner process and proceeds to the next step in the scatter phase.
   */
  template <typename DataT>
  void scatterReduceIter(size_t id);

  /**
   * \brief Handler for receiving scatter-reduce messages.
   *
   * This function handles the data received during the scatter-reduce phase and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  template <typename DataT, typename Scalar>
  void scatterReduceIterHandler(RabenseifnerMsg<Scalar, DataT>* msg);

  /**
   * \brief Check if all gather messages have been received.
   *
   * \return True if all gather messages have been received, false otherwise.
   */
  template <typename DataT>
  bool gatherAllMessagesReceived(size_t id);

  /**
   * \brief Check if the gather phase is complete.
   *
   * \return True if the gather phase is complete, false otherwise.
   */
  template <typename DataT>
  bool gatherIsDone(size_t id);

  /**
   * \brief Check if the gather phase is ready to proceed.
   *
   * \return True if the gather phase is ready to proceed, false otherwise.
   */
  template <typename DataT>
  bool gatherIsReady(size_t id);

  /**
   * \brief Try to reduce the received gather messages.
   *
   * \param step The current step in the gather phase.
   */
  template <typename DataT>
  void gatherTryReduce(size_t id, int32_t step);

  /**
   * \brief Perform the gather iteration.
   *
   * This function sends data to the appropriate partner process and proceeds to the next step in the gather phase.
   */
  template <typename DataT>
  void gatherIter(size_t id);

  /**
   * \brief Handler for receiving gather messages.
   *
   * This function handles the data received during the gather phase and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  template <typename DataT, typename Scalar>
  void gatherIterHandler(RabenseifnerMsg<Scalar, DataT>* msg);

  /**
   * \brief Perform the final part of the allreduce operation.
   *
   * This function completes the allreduce operation, handling any remaining steps and invoking the final handler.
   */
  template <typename DataT>
  void finalPart(size_t id);

  /**
   * \brief Send the result to excluded nodes.
   *
   * This function handles the final step for non-power-of-two process counts, sending the reduced result to excluded nodes.
   */
  template <typename DataT>
  void sendToExcludedNodes(size_t id);

  /**
   * \brief Handler for receiving the final result on excluded nodes.
   *
   * This function handles the data received on excluded nodes and invokes the final handler.
   *
   * \param msg Message containing the final result.
   */
  template <typename DataT, typename Scalar>
  void sendToExcludedNodesHandler(RabenseifnerMsg<Scalar, DataT>* msg);

  vt::objgroup::proxy::Proxy<Rabenseifner> proxy_ = {};

  VirtualProxyType collection_proxy_ = u64empty;
  ObjGroupProxyType objgroup_proxy_ = u64empty;

  size_t local_num_elems_ = {};

  size_t id_ = 0;

  /// Sorted list of Nodes that take part in allreduce
  std::vector<NodeType> nodes_ = {};

  NodeType num_nodes_ = {};

  /// Represents an index inside nodes_
  NodeType this_node_ = {};

  bool is_even_ = false;

  /// Num steps for each scatter/gather phase
  int32_t num_steps_ = {};

  /// 2^num_steps_
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};

  /// For non-power-of-2 number of nodes this respresents whether current Node
  /// is excluded (has value of -1) from computation
  NodeType vrt_node_ = {};

  bool is_part_of_adjustment_group_ = false;

  static inline const std::string name_ = "Rabenseifner";
  static inline constexpr ReducerType type_ = ReducerType::Rabenseifner;
};

} // namespace vt::collective::reduce::allreduce

#include "rabenseifner.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H*/
