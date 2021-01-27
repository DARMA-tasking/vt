/*
//@HEADER
// *****************************************************************************
//
//                            garbage_collect_msg.h
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

#if !defined INCLUDED_VT_EPOCH_GARBAGE_COLLECT_MSG_H
#define INCLUDED_VT_EPOCH_GARBAGE_COLLECT_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/epoch/epoch_garbage_collect.h"

namespace vt { namespace epoch {

/**
 * \struct GarbageCollectMsg
 *
 * \brief Garbage collection message for integral sets that is reduced to find
 * common epochs across nodes which are known to have terminated and thus can be
 * reused.
 */
struct GarbageCollectMsg : vt::collective::ReduceTMsg<IntegralSetData> {
  using MessageParentType = vt::collective::ReduceTMsg<IntegralSetData>;
  vt_msg_serialize_required(); // set_

  GarbageCollectMsg() = default; // required for serialization

  /**
   * \internal \brief Construct a new garbage collection message
   *
   * \param[in] in_epoch the archetype epoch
   * \param[in] in_set the integral set on this node
   */
  GarbageCollectMsg(
    EpochType const& in_epoch, IntegralSet<EpochType> const& in_set
  ) : MessageParentType(IntegralSetData{in_set}),
      epoch_(in_epoch)
  { }

  GarbageCollectMsg(GarbageCollectMsg const&) = default;
  GarbageCollectMsg(GarbageCollectMsg&&) = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | epoch_;
  }

  /**
   * \brief Get the archetype epoch
   *
   * \return the epoch
   */
  EpochType getEpoch() const { return epoch_; }

private:
  EpochType epoch_ = no_epoch; /**< The epoch archetype being collected */
};

/**
 * \struct GarbageConfirmMsg
 *
 * \brief Garbage collection message confirming that all nodes have received the
 * epochs to collect
 */
struct GarbageConfirmMsg : vt::collective::ReduceNoneMsg {
//  vt_msg_serialize_prohibit();

  GarbageConfirmMsg() = default;

  /**
   * \brief Construct a new garbage confirmation message
   *
   * \param[in] in_epoch the epoch
   */
  explicit GarbageConfirmMsg(EpochType in_epoch)
    : epoch_(in_epoch)
  { }

  /**
   * \brief Get the archetype epoch
   *
   * \return the epoch
   */
  EpochType getEpoch() const { return epoch_; }

private:
  EpochType epoch_ = no_epoch; /**< The epoch archetype being collected */
};

}} /* end namespace vt::epoch */

#endif /*INCLUDED_VT_EPOCH_GARBAGE_COLLECT_MSG_H*/
