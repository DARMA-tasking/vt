/*
//@HEADER
// *****************************************************************************
//
//                                 base_unit.h
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

#if !defined INCLUDED_VT_SCHEDULER_BASE_UNIT_H
#define INCLUDED_VT_SCHEDULER_BASE_UNIT_H

#include "vt/config.h"
#include "vt/runnable/runnable.fwd.h"

#include <memory>

namespace vt { namespace sched {

/**
 * \struct BaseUnit
 *
 * \brief The base class for a work unit that either holds a \c std::unique_ptr
 * to a runnable or contains a general lambda to execute.
 */
struct BaseUnit {
  using RunnablePtrType = std::unique_ptr<runnable::RunnableNew>;

  /**
   * \brief Construct with a runnable
   *
   * \param[in] in_r the runnable moved in
   * \param[in] in_is_term whether it's a termination task
   */
  BaseUnit(RunnablePtrType in_r, bool in_is_term)
    : r_(std::move(in_r)),
      is_term_(in_is_term)
  { }

  /**
   * \brief Construct with a general lambda
   *
   * \param[in] in_work the action to execute
   * \param[in] in_is_term whether it's a termination task
   */
  BaseUnit(ActionType in_work, bool in_is_term)
    : work_(in_work),
      is_term_(in_is_term)
  { }

  /**
   * \brief Check if this work unit is a termination task
   *
   * \return whether it's a termination task
   */
  bool isTerm() const { return is_term_; }

  /**
   * \brief Execute the work
   */
  void operator()() { execute(); }

  /**
   * \brief Execute the work
   */
  void execute();

protected:
  RunnablePtrType r_ = nullptr; /**< the runnable task */
  ActionType work_ = nullptr;   /**< the lambda task */
  bool is_term_ = false;        /**< whether it's a termination task */
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_BASE_UNIT_H*/
