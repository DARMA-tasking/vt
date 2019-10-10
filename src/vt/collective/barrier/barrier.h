/*
//@HEADER
// *****************************************************************************
//
//                                  barrier.h
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

struct Barrier : virtual collective::tree::Tree {
  using BarrierStateType = BarrierState;

  Barrier();

  template <typename T>
  using ContainerType = std::unordered_map<BarrierType, T>;

  BarrierStateType& insertFindBarrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    ActionType cont_action = nullptr
  );

  void removeBarrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  BarrierType newNamedBarrier();
  BarrierType newNamedCollectiveBarrier();

  void barrierUp(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    bool const& skip_term
  );

  void barrierDown(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  inline void barrier(
    ActionType poll_action = nullptr, BarrierType const& barrier = no_barrier
  ) {
    return waitBarrier(poll_action, barrier);
  }

  inline void barrierThen(ActionType fn) {
    return contBarrier(fn);
  }

  inline void barrierThen(BarrierType const& barrier, ActionType fn) {
    return contBarrier(fn, barrier);
  }

  inline void systemMetaBarrier() {
    bool const skip_term = true;
    return waitBarrier(nullptr, no_barrier, skip_term);
  }

  inline void systemMetaBarrierCont(ActionType fn) {
    bool const skip_term = true;
    return contBarrier(fn, no_barrier, skip_term);
  }

  static void barrierUp(BarrierMsg* msg);
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
