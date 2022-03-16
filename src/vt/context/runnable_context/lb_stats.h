/*
//@HEADER
// *****************************************************************************
//
//                                  lb_stats.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_STATS_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_STATS_H

#include "vt/context/runnable_context/base.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/elm/elm_stats.fwd.h"

namespace vt { namespace ctx {

/**
 * \struct LBStats
 *
 * \brief Context for collection LB statistics when a task runs
 */
struct LBStats final : Base {
  using ElementIDStruct = elm::ElementIDStruct;
  using ElementStats    = elm::ElementStats;

  /**
   * \brief Construct a \c LBStats
   *
   * \param[in] in_elm the collection element
   * \param[in] msg the incoming message (used for communication stats)
   */
  template <typename ElmT, typename MsgT>
  LBStats(ElmT* in_elm, MsgT* msg);

  /**
   * \brief Construct a \c LBStats
   *
   * \param[in] in_stats the statistics
   * \param[in] in_elm_id the element ID
   */
  LBStats(ElementStats* in_stats, ElementIDStruct const& in_elm_id)
    : stats_(in_stats),
      cur_elm_id_(in_elm_id),
      should_instrument_(true)
  { }

  /**
   * \brief Set the context and timing for a collection task
   */
  void begin() final override;

  /**
   * \brief Remove the context and store timing for a collection task
   */
  void end() final override;

  /**
   * \brief Record statistics whenever a message is sent and a collection
   * element is running.
   *
   * \param[in] dest the destination of the message
   * \param[in] size the size of the message
   */
  void send(elm::ElementIDStruct dest, MsgSizeType bytes) final override;

  void suspend() final override;
  void resume() final override;

  /**
   * \brief Get the current element ID struct for the running context
   *
   * \return the current element ID
   */
  ElementIDStruct const& getCurrentElementID() const;

private:
  ElementStats* stats_ = nullptr;     /**< Element statistics */
  ElementIDStruct cur_elm_id_ = {};   /**< Current element ID  */
  bool should_instrument_ = false;    /**< Whether we are instrumenting */
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_STATS_H*/
