/*
//@HEADER
// *****************************************************************************
//
//                                  barrier.h
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

#if !defined INCLUDED_COLLECTIVE_BARRIER_BARRIER_H
#define INCLUDED_COLLECTIVE_BARRIER_BARRIER_H

#include <unordered_map>

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/tree/tree.h"
#include "vt/collective/barrier/barrier_msg.h"
#include "vt/collective/barrier/barrier_state.h"

namespace vt { namespace collective { namespace barrier {

constexpr BarrierType const fst_barrier = 1;
constexpr BarrierType const fst_coll_barrier = 0x8000000000000001;

/**
 * \struct Barrier
 *
 * \brief Perform a collective barrier that is safe to use with VT handlers in
 * flight.
 *
 * Align execution across multiple nodes to ensure each node across the
 * communicator/runtime reach a "matching" barrier.
 *
 * \warning Barriers are not recommended for users for parallel
 * coordination. First, barriers do not guarantee that work is actually done
 * when the barrier is reached. It only ensures that the node reaches that
 * point. Thus, messages sent or work enqueued for the scheduler might not be
 * done when a barrier is reached. For ensuring that work is complete, use the
 * \c TerminationDetector component to create an epoch that groups the piece of
 * work (see \c vt::runInEpochRooted and \c vt::runInEpochCollective ). Second,
 * barriers may limit the concurrency in a program; in many cases only a
 * reduction is necessary for correctness.
 */
struct Barrier : virtual collective::tree::Tree {
  using BarrierStateType = BarrierState;

  /**
   * \internal \brief Construct a new barrier manager
   */
  Barrier();

  template <typename T>
  using ContainerType = std::unordered_map<BarrierType, T>;

  /**
   * \internal \brief Insert/find a barrier
   *
   * \param[in] is_named whether the barrier is named
   * \param[in] is_wait whether the barrier is of waiting type
   * \param[in] barrier the barrier ID
   * \param[in] cont_action (optional) continuation to attach after completion
   *
   * \return the barrier state
   */
  BarrierStateType& insertFindBarrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    ActionType cont_action = nullptr
  );

  /**
   * \internal \brief Remove the state of a barrier
   *
   * \param[in] is_named whether the barrier is named
   * \param[in] is_wait whether the barrier is of waiting type
   * \param[in] barrier the barrier ID
   */
  void removeBarrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  /**
   * \brief Rooted call to create a new named barrier, returning the ID.
   *
   * After calling this, one must broadcast/send this to other nodes. Typical
   * use case is to put it in a message for later coordination.
   *
   * \return the barrier ID
   */
  BarrierType newNamedBarrier();

  /**
   * \brief Collective call to create a new named barrier, returning the ID.
   *
   * All nodes must call this in the same order to generate a consistent barrier
   * ID for waiting on later.
   *
   * \return the barrier ID
   */
  BarrierType newNamedCollectiveBarrier();

  /**
   * \internal \brief Send a barrier up the tree
   *
   * \param[in] is_named whether the barrier is named
   * \param[in] is_wait whether the barrier is of waiting type
   * \param[in] barrier the barrier ID
   * \param[in] skip_term whether to skip termination (mark barrier a TD message)
   */
  void barrierUp(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    bool const& skip_term
  );

  /**
   * \internal \brief Send a barrier down the tree to release nodes (barrier is
   * reached!)
   *
   * \param[in] is_named whether the barrier is named
   * \param[in] is_wait whether the barrier is of waiting type
   * \param[in] barrier the barrier ID
   */
  void barrierDown(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  /**
   * \brief Wait on a barrier
   *
   * \param[in] poll_action action to execute while polling for barrier
   * completion in addition to polling scheduler
   * \param[in] barrier the barrier ID to wait on
   */
  inline void barrier(
    ActionType poll_action = nullptr, BarrierType const& barrier = no_barrier
  ) {
    return waitBarrier(poll_action, barrier);
  }

  /**
   * \brief Collectively create a new barrier and once completed execute an
   * action.
   *
   * \param[in] fn the action to execute after the barrier is reached by all
   * nodes
   */
  inline void barrierThen(ActionType fn) {
    return contBarrier(fn);
  }

  /**
   * \brief Collective wait for a barrier and once completed execute an action.
   *
   * \param[in] barrier the barrier to wait on
   * \param[in] fn the action to execute after the barrier is reached by all
   * nodes
   */
  inline void barrierThen(BarrierType const& barrier, ActionType fn) {
    return contBarrier(fn, barrier);
  }

  /**
   * \internal \brief Collectively barrier skipping termination
   *
   * \warning This is dangerous to call in any code outside of internal
   * initialize and finalize.
   */
  inline void systemMetaBarrier() {
    bool const skip_term = true;
    return waitBarrier(nullptr, no_barrier, skip_term);
  }

  /**
   * \internal \brief Collectively barrier skipping termination and then execute
   * a continuation.
   *
   * \param[in] fn action to execute after barrier is reached
   *
   * \warning This is dangerous to call in any code outside of internal
   * initialize and finalize.
   */
  inline void systemMetaBarrierCont(ActionType fn) {
    bool const skip_term = true;
    return contBarrier(fn, no_barrier, skip_term);
  }

  /**
   * \internal \brief Active handler to send a barrier up the spanning tree
   *
   * \param[in] msg the barrier message
   */
  static void barrierUp(BarrierMsg* msg);

  /**
   * \internal \brief Active handler to send a barrier down the spanning tree
   *
   * \param[in] msg the barrier message
   */
  static void barrierDown(BarrierMsg* msg);

private:

  void waitBarrier(
    ActionType poll_action = nullptr, BarrierType const& barrier = no_barrier,
    bool const skip_term = false
  );

  void contBarrier(
    ActionType fn, BarrierType const& barrier = no_barrier,
    bool const skip_term = false
  );

private:
  BarrierType cur_named_barrier_ = fst_barrier;
  BarrierType cur_named_coll_barrier_ = fst_coll_barrier;
  BarrierType cur_unnamed_barrier_ = fst_barrier;

  ContainerType<BarrierStateType> named_barrier_state_, unnamed_barrier_state_;
};

}}}  // end namespace vt::collective::barrier

#endif /*INCLUDED_COLLECTIVE_BARRIER_BARRIER_H*/
