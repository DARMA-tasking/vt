/*
//@HEADER
// *****************************************************************************
//
//                                    base.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_BASE_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_BASE_H

#include "vt/configs/types/types_type.h"
#include "vt/elm/elm_id.h"

namespace vt { namespace ctx {

/**
 * \struct Base
 *
 * \brief Base context for runnable tasks.
 *
 * \c ctx::Base is used to create contexts that are associated with tasks
 * wrapped with the \c runnable::Runnable class. When messages arrive and
 * trigger a handler or other actions occur, contexts that inherit from \c Base
 * can be used to maintain a particular context when that runnable is passed to
 * the scheduler for later execution. The \c begin() and \c end() methods are
 * called when the task starts and stops. If VT is built with user-level threads
 * (ULTs), \c suspend() and \c resume might be called if the thread that a task
 * is running in suspends the stack mid-execution (typically waiting for a
 * dependency). Thus, any context is expected to save all state in suspend and
 * then return that state back during resume when the ULT is resumed.
 *
 * \warning Note that contexts should not hold on to a message pointer and read
 * values from the message after the \c begin() method is called as the user
 * might modify the message or forward it.
 */
struct Base {

  virtual ~Base() = default;

  /**
   * \brief Invoked immediately before a task is executed
   */
  virtual void begin() {}

  /**
   * \brief Invoked immediately after a task is executed
   */
  virtual void end() {}

  /**
   * \brief Invoked when a task is suspended (for ULTs, when enabled)
   */
  virtual void suspend() {}

  /**
   * \brief Invoked when a handler is resumed (for ULTs, when enabled)
   */
  virtual void resume() {}

  /**
   * \brief Invoked when a message is sent to any node
   *
   * \param[in] dest the destination of the message
   * \param[in] size the size of the message
   */
  virtual void send(elm::ElementIDStruct dest, MsgSizeType bytes) { }
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_BASE_H*/
