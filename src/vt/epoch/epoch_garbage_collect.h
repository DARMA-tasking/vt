/*
//@HEADER
// *****************************************************************************
//
//                           epoch_garbage_collect.h
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

#if !defined INCLUDED_VT_EPOCH_EPOCH_GARBAGE_COLLECT_H
#define INCLUDED_VT_EPOCH_EPOCH_GARBAGE_COLLECT_H

#include "vt/config.h"
#include "vt/termination/interval/integral_set_intersect.h"
#include "vt/messaging/message/smart_ptr.h"

namespace vt { namespace epoch {

/**
 * \struct IntegralSetData
 *
 * \brief Container that holds a \c IntegralSet<EpochType> to perform set
 * intersection for a garbage collecting reducer
 */
struct IntegralSetData {
  using IntegralSetType = vt::IntegralSet<EpochType>;

  IntegralSetData() = default;

  /**
   * \internal \brief Construct the container
   *
   * \param[in] in_set the integral set of this node
   */
  explicit IntegralSetData(IntegralSetType const& in_set)
    : set_(in_set)
  { }

  /**
   * \internal \brief Intersect two integral sets to determine common epochs
   * that can be garbage collected
   *
   * \param[in] a1 integral set 1
   * \param[in] a2 integral set 2
   *
   * \return the intersection of 1 and 2
   */
  friend IntegralSetData operator+(IntegralSetData a1, IntegralSetData const& a2) {
    return IntegralSetData{
      termination::interval::Intersect<IntegralSetType>()(a1.set_, a2.set_)
    };
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | set_;
  }

  /**
   * \brief Get the underlying integral set
   *
   * \return the integral set
   */
  IntegralSetType const& getSet() const {
    return set_;
  }

private:
  IntegralSetType set_;         /**< The integral set */
};

// fwd-declare the garbage collect messages to reduce dependencies
struct GarbageCollectMsg;
struct GarbageConfirmMsg;

/**
 * \struct GarbageCollectTrait
 *
 * \brief Garbage collection functionality
 */
struct GarbageCollectTrait {

  /**
   * \internal \brief Reduce handler for a set of epochs being garbage collected
   *
   * \param[in] msg the set of epoch
   */
  void reducedEpochsImpl(GarbageCollectMsg* msg);

};

}} /* end namespace vt::epoch */

#endif /*INCLUDED_VT_EPOCH_EPOCH_GARBAGE_COLLECT_H*/
