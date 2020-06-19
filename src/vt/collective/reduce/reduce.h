/*
//@HEADER
// *****************************************************************************
//
//                                   reduce.h
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

#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.fwd.h"
#include "vt/collective/reduce/reduce_hash.h"
#include "vt/collective/reduce/reduce_state.h"
#include "vt/collective/reduce/reduce_state_holder.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/collective/reduce/operators/callback_op.h"
#include "vt/messaging/active.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/tree/tree.h"
#include "vt/utils/hash/hash_tuple.h"

#include <tuple>
#include <unordered_map>
#include <cassert>
#include <cstdint>

namespace vt { namespace collective { namespace reduce {

/**
 * \struct Reduce
 *
 * \brief A specific, isolated reducer instance for a given scope that sequences
 * reduce operations via the reduction stamp within that scope.
 *
 * Holds the state as a reduction makes it up the spanning tree until it reaches
 * the root. Combines messages with the user-specified reduction operator as
 * they move up the spanning tree.
 */
struct Reduce : virtual collective::tree::Tree {
  using ReduceStateType = ReduceState;
  using ReduceNumType   = typename ReduceStateType::ReduceNumType;

  /**
   * \internal \brief Construct a new reducer instance
   *
   * \param[in] in_scope the scope for the reducer
   */
  explicit Reduce(detail::ReduceScope const& in_scope);

  /**
   * \internal \brief Construct a new reducer instance with a custom
   * (or non-global) spanning tree
   *
   * \param[in] in_scope the scope for the reducer
   * \param[in] in_tree the spanning tree
   */
  Reduce(
    detail::ReduceScope const& in_scope, collective::tree::Tree* in_tree
  );

  /**
   * \internal \brief Generate the next reduction stamp
   *
   * \return the stamp
   */
  detail::ReduceStamp generateNextID();

  /**
   * \brief Reduce a message up the tree
   *
   * \param[in] root the root node where the final handler provides the result
   * \param[in] msg the message to reduce on this node
   * \param[in] id the reduction stamp (optional), provided if out-of-order
   * \param[in] num_contrib number of expected contributions from this node
   *
   * \return the next reduction stamp
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  detail::ReduceStamp reduce(
    NodeType root, MsgT* const msg,
    detail::ReduceStamp id = detail::ReduceStamp{},
    ReduceNumType num_contrib = 1
  );

  /**
   * \brief Reduce a message up the tree
   *
   * \param[in] root the root node where the final handler provides the result
   * \param[in] msg the message to reduce on this node
   * \param[in] cb the callback to trigger on the root node
   * \param[in] id the reduction stamp (optional), provided if out-of-order
   * \param[in] num_contrib number of expected contributions from this node
   *
   * \return the next reduction stamp
   */
  template <
    typename OpT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<
      MsgT, OpT, collective::reduce::operators::ReduceCallback<MsgT>
    >
  >
  detail::ReduceStamp reduce(
    NodeType const& root, MsgT* msg, Callback<MsgT> cb,
    detail::ReduceStamp id = detail::ReduceStamp{},
    ReduceNumType const& num_contrib = 1
  );

  /**
   * \brief Reduce a message up the tree with a target function on the root node
   *
   * \param[in] root the root node where the final handler provides the result
   * \param[in] msg the message to reduce on this node
   * \param[in] id the reduction stamp (optional), provided if out-of-order
   * \param[in] num_contrib number of expected contributions from this node
   *
   * \return the next reduction stamp
   */
  template <
    typename OpT,
    typename FunctorT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<MsgT, OpT, FunctorT>
  >
  detail::ReduceStamp reduce(
    NodeType const& root, MsgT* msg,
    detail::ReduceStamp id = detail::ReduceStamp{},
    ReduceNumType const& num_contrib = 1
  );

  /**
   * \internal \brief Combine in a new message for a given reduction
   *
   * \param[in] msg the message to combine with the operator
   * \param[in] local was this called locally or from an incoming handler
   * \param[in] num_contrib the number of expected local contributions
   */
  template <typename MsgT>
  void reduceAddMsg(
    MsgT* msg, bool const local, ReduceNumType num_contrib = -1
  );

  /**
   * \internal \brief Combine and send up the tree if ready
   *
   * \param[in] msg the message to reduce
   */
  template <typename MsgT>
  void reduceNewMsg(MsgT* msg);

  /**
   * \internal \brief Explicitly start the reduction when the number of
   *  contributions is not known up front
   *
   * \param[in] id the reduce stamp to start
   * \param[in] use_num_contrib whether to use the cached # of contributions
   */
  template <typename MsgT>
  void startReduce(detail::ReduceStamp id, bool use_num_contrib = true);

  /**
   * \internal \brief Active function when a message reaches the root of the
   * spanning tree and the reduction is complete
   *
   * \param[in] msg the reduce message
   */
  template <typename MsgT>
  void reduceRootRecv(MsgT* msg);

  /**
   * \internal \brief Active function when a message arrives for a given scope
   * at some level in the spanning tree
   *
   * \param[in] msg the reduce message
   */
  template <typename MsgT>
  void reduceUp(MsgT* msg);

private:
  detail::ReduceScope scope_;   /**< The reduce scope for this reducer */
  ReduceStateHolder state_;     /**< Reduce state, holds messages, etc. */
  detail::StrongSeq next_seq_;  /**< The next reduce stamp */
};

}}} /* end namespace vt::collective::reduce */

#include "vt/collective/reduce/reduce.impl.h"

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_H*/
