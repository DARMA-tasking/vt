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

#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "type.h"
#include "vt/configs/types/types_type.h"
#include "vt/collective/reduce/allreduce/recursive_doubling_msg.h"

namespace vt::collective::reduce::allreduce {

/**
 * \brief Class implementing the Recursive Doubling algorithm for allreduce operation.
 *
 * This class provides an implementation of the Recursive Doubling algorithm for the
 * allreduce operation. It is parameterized by the data type to be reduced, the reduction
 * operation, the object type, and the final handler.
 */

struct RecursiveDoubling {

  /**
   * \brief Constructor for Collection
   *
   * \param proxy Collection proxy
   * \param group GroupID (for given collection)
   * \param num_elems Number of local collection elements
   */
  RecursiveDoubling(detail::StrongVrtProxy proxy, detail::StrongGroup group, size_t num_elems);

  /**
   * \brief Constructor for ObjGroup
   *
   * \param objgroup ObjGroupProxy
   */
  RecursiveDoubling(detail::StrongObjGroup objgroup);

  /**
   * \brief Constructor for Group
   *
   * \param group GroupID
   */
  RecursiveDoubling(detail::StrongGroup group);

  ~RecursiveDoubling();

  /**
   * \brief Execute the final handler callback with the reduced result.
   *
   * \param id Allreduce ID
   */
  template <typename DataT>
  void executeFinalHan(size_t id);

  /**
   * \brief Set final handler that will be executed with allreduce result
   *
   * \param fin Callback to be executed
   * \param id Allreduce ID
   */
  template <typename DataT, typename CallbackType>
  void setFinalHandler(const CallbackType& fin, size_t id);

  /**
   * \brief Performs local reduce, and once the local one is done it starts up the global allreduce
   *
   * \param id Allreduce ID
   * \param args Data to be allreduced
   */
  template <typename DataT, template <typename Arg> class Op, typename... Args>
  void localReduce(size_t id, Args&&... args);

  /**
   * \brief Start the allreduce operation.
   *
   * \param id Allreduce ID
   */
  template <typename DataT, template <typename Arg> class Op>
  void allreduce(size_t id);

  /**
   * \brief Initialize the RecursiveDoubling object.
   *
   * \param id Allreduce ID
   * \param args Additional arguments for data initialization.
   */
  template <typename DataT, typename ...Args>
  void initialize(size_t id, Args&&... data);

  /**
   * \brief Initialize the internal state of allreduce algorithm.
   *
   * \param id Allreduce ID
   */
  template <typename DataT>
  void initializeState(size_t id);

  /**
   * \brief Adjust for power of two nodes.
   *
   * \param id Allreduce ID
   */
  template <typename DataT, template <typename Arg> class Op>
  void adjustForPowerOfTwo(size_t id);

  /**
   * \brief Handler for adjusting for power of two nodes.
   *
   * \param msg Pointer to the message.
   */
  template <typename DataT, template <typename Arg> class Op>
  void adjustForPowerOfTwoHandler(RecursiveDoublingMsg<DataT>* msg);

  /**
   * \brief Check if the allreduce operation is done.
   *
   * \param id Allreduce ID
   * \return True if the operation is done, otherwise false.
   */
  template <typename DataT>
  bool isDone(size_t id);

  /**
   * \brief Check if the current state is valid for allreduce.
   *
   * \param id Allreduce ID
   * \return True if the state is valid, otherwise false.
   */
  template <typename DataT>
  bool isValid(size_t id);

  /**
   * \brief Check if all messages are received for the current step.
   *
   * \param id Allreduce ID
   * \return True if all messages are received, otherwise false.
   */
  template <typename DataT>
  bool allMessagesReceived(size_t id);

  /**
   * \brief Check if the object is ready for the next step of allreduce.
   *
   * \param id Allreduce ID
   * \return True if ready, otherwise false.
   */
  template <typename DataT>
  bool isReady(size_t id);

  /**
   * \brief Perform the next step of the allreduce operation.
   *
   * \param id Allreduce ID
   */
  template <typename DataT, template <typename Arg> class Op>
  void reduceIter(size_t id);

  /**
   * \brief Try to reduce the message at the specified step.
   *
   * \param id Allreduce ID
   * \param step The step at which to try reduction.
   */
  template <typename DataT, template <typename Arg> class Op>
  void tryReduce(size_t id, int32_t step);

  /**
   * \brief Handler for the reduce iteration.
   *
   * \param msg Pointer to the message.
   */
  template <typename DataT, template <typename Arg> class Op>
  void reduceIterHandler(RecursiveDoublingMsg<DataT>* msg);

  /**
   * \brief Send data to excluded nodes for finalization.
   *
   * \param id Allreduce ID
   */
  template <typename DataT>
  void sendToExcludedNodes(size_t id);

  /**
   * \brief Handler for sending data to excluded nodes.
   *
   * \param msg Pointer to the message.
   */
  template <typename DataT>
  void sendToExcludedNodesHandler(RecursiveDoublingMsg<DataT>* msg);

  /**
   * \brief Perform the final part of the allreduce operation.
   *
   * \param id Allreduce ID
   */
  template <typename DataT>
  void finalPart(size_t id);

  vt::objgroup::proxy::Proxy<RecursiveDoubling> proxy_ = {};

  VirtualProxyType collection_proxy_ = u64empty;
  ObjGroupProxyType objgroup_proxy_ = u64empty;
  GroupType group_ = u64empty;

  size_t local_num_elems_ = {};

  std::vector<NodeType> nodes_ = {};
  NodeType num_nodes_ = {};
  NodeType this_node_ = {};

  bool is_even_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};

  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
  static inline ReducerType type_ = ReducerType::RecursiveDoubling;
};

} // namespace vt::collective::reduce::allreduce

#include "recursive_doubling.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H*/
