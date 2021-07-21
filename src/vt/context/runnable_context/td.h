/*
//@HEADER
// *****************************************************************************
//
//                                     td.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TD_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TD_H

#include "vt/context/runnable_context/base.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"

#include <vector>

namespace vt { namespace ctx {

/**
 * \struct TD
 *
 * \brief Context for termination detection to be preserved with a task. Manages
 * the epoch stack  associated with running tasks. Produces and consumes in the
 * constructor and destructor to ensure termination is not detected early.
 */
struct TD final : Base {

  /**
   * \brief Construct with a given epoch; produce on that epoch.
   *
   * \param[in] in_ep the epoch
   */
  explicit TD(EpochType in_ep);

  /**
   * \brief When destroyed, consume the epoch held by the context.
   */
  virtual ~TD();

  /**
   * \brief Construct with a message to extract the epoch; produce on that
   * epoch.
   *
   * \param[in] msg the message to extract the epoch from
   */
  template <typename MsgPtrT>
  explicit TD(MsgPtrT msg);

  /**
   * \brief During begin \c TD will capture the epoch stack size and push \c ep_
   */
  void begin() final override;

  /**
   * \brief During end \c TD will pop all epochs off of the stack down to the
   * size in captured in \c begin()
   */
  void end() final override;

  /**
   * \brief When suspended, \c TD will preserve any epochs pushed on the stack
   * after begin and restore the stack back to the state before begin was
   * invoked
   */
  void suspend() final override;

  /**
   * \brief When resumed, \c TD will restore the stack back from when it was
   * suspended
   */
  void resume() final override;

private:
  EpochType ep_ = no_epoch;                    /**< The epoch for the task */
  std::size_t base_epoch_stack_size_ = 0;      /**< Epoch stack size at start  */
  std::vector<EpochType> suspended_epochs_;    /**< Suspended epoch stack */
};

}} /* end namespace vt::ctx */

#include "vt/context/runnable_context/td.impl.h"

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TD_H*/
