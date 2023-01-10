/*
//@HEADER
// *****************************************************************************
//
//                                  lb_data.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_H

#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/elm/elm_lb_data.fwd.h"

namespace vt { namespace ctx {

/**
 * \struct LBData
 *
 * \brief Context for collection of LB data when a task runs
 */
struct LBData {
  using ElementIDStruct = elm::ElementIDStruct;
  using ElementLBData    = elm::ElementLBData;

  LBData() = default;

  /**
   * \brief Construct a \c LBData
   *
   * \param[in] in_elm the collection element
   * \param[in] msg the incoming message (used for communication LB data)
   */
  template <typename ElmT, typename MsgT>
  LBData(ElmT* in_elm, MsgT* msg);

  /**
   * \brief Construct a \c LBData
   *
   * \param[in] in_lb_data the LB data
   * \param[in] in_elm_id the element ID
   */
  LBData(ElementLBData* in_lb_data, ElementIDStruct const& in_elm_id)
    : lb_data_(in_lb_data),
      cur_elm_id_(in_elm_id),
      should_instrument_(true)
  { }

  /**
   * \brief Return whether time is required
   */
  bool needsTime() const { return should_instrument_; }

  /**
   * \brief Set the context and timing for a collection task
   */
  void start(TimeType time);

  /**
   * \brief Remove the context and store timing for a collection task
   */
  void finish(TimeType time);

  /**
   * \brief Record LB data whenever a message is sent and a collection
   * element is running.
   *
   * \param[in] dest the destination of the message
   * \param[in] size the size of the message
   */
  void send(elm::ElementIDStruct dest, MsgSizeType bytes);

  void suspend(TimeType time);
  void resume(TimeType time);

  /**
   * \brief Get the current element ID struct for the running context
   *
   * \return the current element ID
   */
  ElementIDStruct const& getCurrentElementID() const;

private:
  ElementLBData* lb_data_ = nullptr;     /**< Element LB data */
  ElementIDStruct cur_elm_id_ = {};   /**< Current element ID  */
  bool should_instrument_ = false;    /**< Whether we are instrumenting */
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_H*/
