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

#include <cstdint>

namespace vt::collective::reduce::allreduce {

template <typename Scalar>
struct AllreduceRbnRawMsg
  : Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required();


  AllreduceRbnRawMsg() = default;
  AllreduceRbnRawMsg(AllreduceRbnRawMsg const&) = default;
  AllreduceRbnRawMsg(AllreduceRbnRawMsg&&) = default;
  ~AllreduceRbnRawMsg() {
    if (owning_) {
      delete[] val_;
    }
  }

  AllreduceRbnRawMsg(Scalar* in_val, size_t size, size_t id, int step = 0)
    : MessageParentType(),
      val_(in_val),
      size_(size),
      id_(id),
      step_(step) { }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
      MessageParentType::serialize(s);

      s | size_;

      if (s.isUnpacking()) {
        owning_ = true;
        val_ = new Scalar[size_];
      }

      checkpoint::dispatch::serializeArray(s, val_, size_);

      s | id_;
      s | step_;
  }

  Scalar* val_ = {};
  size_t size_ = {};
  size_t id_ = {};
  int32_t step_ = {};
  bool owning_ = false;
};

/**
 * \struct Rabenseifner
 * \brief Class implementing Rabenseifner's allreduce algorithm.
 *
 * This class performs an allreduce operation using Rabenseifner's method. The algorithm consists
 * of several phases: adjustment for power-of-two processes, scatter-reduce, and gather-allgather.
 *
 * \tparam DataT Type of the data being reduced.
 * \tparam Op Reduction operation (e.g., sum, max, min).
 * \tparam ObjT Object type used for callback invocation.
 * \tparam finalHandler Callback handler for the final result.
 */
template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
struct Rabenseifner {
  using DataType = DataHandler<DataT>;
  using Scalar = typename DataType::Scalar;

  /**
   * \brief Constructor for Rabenseifner's allreduce algorithm.
   *
   * \param parentProxy Proxy to the object group managing the reduction.
   * \param num_nodes Total number of nodes involved in the allreduce operation.
   * \param args Additional arguments for initializing the data value.
   */
  template <typename ...Args>
  Rabenseifner(
    vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
    Args&&... args);

  /**
   * \brief Initialize the allreduce algorithm.
   *
   * This function sets up the necessary data structures and initial values for the reduction operation.
   *
   * \param args Additional arguments for initializing the data value.
   */
  template <typename ...Args>
  void initialize(size_t id, Args&&... args);

  void initializeState(size_t id);
  size_t generateNewId() { return id_++; }

  /**
   * \brief Execute the final handler callback with the reduced result.
   */
  void executeFinalHan(size_t id);

  /**
   * \brief Perform the allreduce operation.
   *
   * This function starts the allreduce operation, adjusting for non-power-of-two process counts if necessary.
   */
  void allreduce(size_t id);

  /**
   * \brief Adjust the process count to the nearest power-of-two.
   *
   * This function performs additional steps to handle non-power-of-two process counts, ensuring that the
   * main scatter-reduce and gather-allgather phases can proceed with a power-of-two number of processes.
   */
  void adjustForPowerOfTwo(size_t id);

  /**
   * \brief Handler for adjusting the right half of the process group.
   *
   * This function handles the data received from the partner process and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  void adjustForPowerOfTwoRightHalf(AllreduceRbnRawMsg<Scalar>* msg);

  /**
   * \brief Handler for adjusting the left half of the process group.
   *
   * This function handles the data received from the partner process and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  void adjustForPowerOfTwoLeftHalf(AllreduceRbnRawMsg<Scalar>* msg);

  /**
   * \brief Final adjustment step for non-power-of-two process counts.
   *
   * This function handles the final step of the adjustment phase, combining the data and proceeding to the next phase.
   *
   * \param msg Message containing the data from the partner process.
   */
  void adjustForPowerOfTwoFinalPart(AllreduceRbnRawMsg<Scalar>* msg);

  /**
   * \brief Check if all scatter messages have been received.
   *
   * \return True if all scatter messages have been received, false otherwise.
   */
  bool scatterAllMessagesReceived(size_t id);

  /**
   * \brief Check if the scatter phase is complete.
   *
   * \return True if the scatter phase is complete, false otherwise.
   */
  bool scatterIsDone(size_t id);

  /**
   * \brief Check if the scatter phase is ready to proceed.
   *
   * \return True if the scatter phase is ready to proceed, false otherwise.
   */
  bool scatterIsReady(size_t id);

  /**
   * \brief Try to reduce the received scatter messages.
   *
   * \param step The current step in the scatter phase.
   */
  void scatterTryReduce(size_t id, int32_t step);

  /**
   * \brief Perform the scatter-reduce iteration.
   *
   * This function sends data to the appropriate partner process and proceeds to the next step in the scatter phase.
   */
  void scatterReduceIter(size_t id);

  /**
   * \brief Handler for receiving scatter-reduce messages.
   *
   * This function handles the data received during the scatter-reduce phase and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  void scatterReduceIterHandler(AllreduceRbnRawMsg<Scalar>* msg);

  /**
   * \brief Check if all gather messages have been received.
   *
   * \return True if all gather messages have been received, false otherwise.
   */
  bool gatherAllMessagesReceived(size_t id);

  /**
   * \brief Check if the gather phase is complete.
   *
   * \return True if the gather phase is complete, false otherwise.
   */
  bool gatherIsDone(size_t id);

  /**
   * \brief Check if the gather phase is ready to proceed.
   *
   * \return True if the gather phase is ready to proceed, false otherwise.
   */
  bool gatherIsReady(size_t id);

  /**
   * \brief Try to reduce the received gather messages.
   *
   * \param step The current step in the gather phase.
   */
  void gatherTryReduce(size_t id, int32_t step);

  /**
   * \brief Perform the gather iteration.
   *
   * This function sends data to the appropriate partner process and proceeds to the next step in the gather phase.
   */
  void gatherIter(size_t id);

  /**
   * \brief Handler for receiving gather messages.
   *
   * This function handles the data received during the gather phase and combines it using the reduction operation.
   *
   * \param msg Message containing the data from the partner process.
   */
  void gatherIterHandler(AllreduceRbnRawMsg<Scalar>* msg);

  /**
   * \brief Perform the final part of the allreduce operation.
   *
   * This function completes the allreduce operation, handling any remaining steps and invoking the final handler.
   */
  void finalPart(size_t id);

  /**
   * \brief Send the result to excluded nodes.
   *
   * This function handles the final step for non-power-of-two process counts, sending the reduced result to excluded nodes.
   */
  void sendToExcludedNodes(size_t id);

  /**
   * \brief Handler for receiving the final result on excluded nodes.
   *
   * This function handles the data received on excluded nodes and invokes the final handler.
   *
   * \param msg Message containing the final result.
   */
  void sendToExcludedNodesHandler(AllreduceRbnRawMsg<Scalar>* msg);

  vt::objgroup::proxy::Proxy<Rabenseifner> proxy_ = {};
  vt::objgroup::proxy::Proxy<ObjT> parent_proxy_ = {};

  struct State {
      std::vector<Scalar> val_ = {};
      size_t size_ = {};

      bool finished_adjustment_part_ = false;
      MsgSharedPtr<AllreduceRbnRawMsg<Scalar>> left_adjust_message_ = nullptr;
      MsgSharedPtr<AllreduceRbnRawMsg<Scalar>> right_adjust_message_ = nullptr;

      int32_t mask_ = 1;
      int32_t step_ = 0;
      bool initialized_ = false;
      bool completed_ = false;

      // Scatter
      int32_t scatter_mask_ = 1;
      int32_t scatter_step_ = 0;
      int32_t scatter_num_recv_ = 0;
      std::vector<bool> scatter_steps_recv_ = {};
      std::vector<bool> scatter_steps_reduced_ = {};
      std::vector<MsgSharedPtr<AllreduceRbnRawMsg<Scalar>>> scatter_messages_ =
        {};
      bool finished_scatter_part_ = false;

      // Gather
      int32_t gather_step_ = 0;
      int32_t gather_mask_ = 1;
      int32_t gather_num_recv_ = 0;
      std::vector<bool> gather_steps_recv_ = {};
      std::vector<bool> gather_steps_reduced_ = {};
      std::vector<MsgSharedPtr<AllreduceRbnRawMsg<Scalar>>> gather_messages_ =
        {};

      std::vector<uint32_t> r_index_ = {};
      std::vector<uint32_t> r_count_ = {};
      std::vector<uint32_t> s_index_ = {};
      std::vector<uint32_t> s_count_ = {};
  };

  size_t id_ = 0;
  std::unordered_map<size_t, State> states_ = {};
  NodeType num_nodes_ = {};
  NodeType this_node_ = {};

  bool is_even_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};

  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
};

} // namespace vt::collective::reduce::allreduce

#include "rabenseifner.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H*/
