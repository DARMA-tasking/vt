/*
//@HEADER
// *****************************************************************************
//
//                             recursive_doubling.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/message/message.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/configs/error/config_assert.h"
#include "vt/messaging/message/smart_ptr.h"
#include "data_handler.h"

#include <tuple>
#include <cstdint>

namespace vt::collective::reduce::allreduce {

template <typename DataT>
struct AllreduceDblMsg
  : SerializeIfNeeded<vt::Message, AllreduceDblMsg<DataT>, DataT> {
  using MessageParentType =
    SerializeIfNeeded<::vt::Message, AllreduceDblMsg<DataT>, DataT>;

  AllreduceDblMsg() = default;
  AllreduceDblMsg(AllreduceDblMsg const&) = default;
  AllreduceDblMsg(AllreduceDblMsg&&) = default;

  AllreduceDblMsg(DataT&& in_val, int step = 0)
    : MessageParentType(),
      val_(std::forward<DataT>(in_val)),
      step_(step) { }
  AllreduceDblMsg(DataT const& in_val, int step = 0)
    : MessageParentType(),
      val_(in_val),
      step_(step) { }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
    s | val_;
    s | step_;
  }

  DataT val_ = {};
  int32_t step_ = {};
};

template <typename Scalar>
struct AllreduceDblRawMsg
  : Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required();


  AllreduceDblRawMsg() = default;
  AllreduceDblRawMsg(AllreduceDblRawMsg const&) = default;
  AllreduceDblRawMsg(AllreduceDblRawMsg&&) = default;
  ~AllreduceDblRawMsg() {
    if (owning_) {
      delete[] val_;
    }
  }

  AllreduceDblRawMsg(std::vector<Scalar>& in_val, int step = 0)
    : MessageParentType(),
      val_(in_val.data()),
      size_(in_val.size()),
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

      s | step_;
  }

  Scalar* val_ = {};
  size_t size_ = {};
  int32_t step_ = {};
  bool owning_ = false;
};

/**
 * \brief Class implementing the Recursive Doubling algorithm for allreduce operation.
 *
 * This class provides an implementation of the Recursive Doubling algorithm for the
 * allreduce operation. It is parameterized by the data type to be reduced, the reduction
 * operation, the object type, and the final handler.
 *
 * \tparam DataT The data type to be reduced.
 * \tparam Op The reduction operation type.
 * \tparam ObjT The object type.
 * \tparam finalHandler The final handler.
 */
template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
struct RecursiveDoubling {
  using DataType = DataHandler<DataT>;
  using Scalar = typename DataHandler<DataT>::Scalar;
  /**
   * \brief Constructor for RecursiveDoubling class.
   *
   * Initializes the RecursiveDoubling object with the provided parameters.
   *
   * \param parentProxy The parent proxy.
   * \param num_nodes The number of nodes.
   * \param args Additional arguments for data initialization.
   */
  RecursiveDoubling(
    vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
    const DataT& data);

  /**
   * \brief Start the allreduce operation.
   */
  void allreduce();

  /**
   * \brief Initialize the RecursiveDoubling object.
   *
   * \param args Additional arguments for data initialization.
   */
  void initialize(const DataT& data);

  /**
   * \brief Adjust for power of two nodes.
   */
  void adjustForPowerOfTwo();

  /**
   * \brief Handler for adjusting for power of two nodes.
   *
   * \param msg Pointer to the message.
   */
  void adjustForPowerOfTwoHandler(AllreduceDblRawMsg<Scalar>* msg);

  /**
   * \brief Check if the allreduce operation is done.
   *
   * \return True if the operation is done, otherwise false.
   */
  bool done();

  /**
   * \brief Check if the current state is valid for allreduce.
   *
   * \return True if the state is valid, otherwise false.
   */
  bool isValid();

  /**
   * \brief Check if all messages are received for the current step.
   *
   * \return True if all messages are received, otherwise false.
   */
  bool allMessagesReceived();

  /**
   * \brief Check if the object is ready for the next step of allreduce.
   *
   * \return True if ready, otherwise false.
   */
  bool isReady();

  /**
   * \brief Perform the next step of the allreduce operation.
   */
  void reduceIter();

  /**
   * \brief Try to reduce the message at the specified step.
   *
   * \param step The step at which to try reduction.
   */
  void tryReduce(int32_t step);

  /**
   * \brief Handler for the reduce iteration.
   *
   * \param msg Pointer to the message.
   */
  void reduceIterHandler(AllreduceDblRawMsg<Scalar>* msg);

  /**
   * \brief Send data to excluded nodes for finalization.
   */
  void sendToExcludedNodes();

  /**
   * \brief Handler for sending data to excluded nodes.
   *
   * \param msg Pointer to the message.
   */
  void sendToExcludedNodesHandler(AllreduceDblRawMsg<Scalar>* msg);

  /**
   * \brief Perform the final part of the allreduce operation.
   */
  void finalPart();

  vt::objgroup::proxy::Proxy<RecursiveDoubling> proxy_ = {};
  vt::objgroup::proxy::Proxy<ObjT> parent_proxy_ = {};

  // DataT val_ = {};
  std::vector<Scalar> val_;
  NodeType num_nodes_ = {};
  NodeType this_node_ = {};

  bool is_even_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};

  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
  bool finished_adjustment_part_ = false;

  int32_t mask_ = 1;
  int32_t step_ = 0;

  bool completed_ = false;

  std::vector<bool> steps_recv_ = {};
  std::vector<bool> steps_reduced_ = {};

  std::vector<MsgSharedPtr<AllreduceDblRawMsg<Scalar>>> messages_ = {};
};

} // namespace vt::collective::reduce::allreduce

#include "recursive_doubling.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H*/
