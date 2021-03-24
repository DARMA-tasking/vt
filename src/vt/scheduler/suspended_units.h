/*
//@HEADER
// *****************************************************************************
//
//                              suspended_units.h
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

#if !defined INCLUDED_VT_SCHEDULER_SUSPENDED_UNITS_H
#define INCLUDED_VT_SCHEDULER_SUSPENDED_UNITS_H

#include "vt/configs/types/types_type.h"
#include "vt/scheduler/priority.h"
#include "vt/runnable/runnable.fwd.h"

#include <unordered_map>
#include <memory>

namespace vt { namespace sched {

namespace detail {

/**
 * \internal \struct SuspendedRunnable
 *
 * \brief A suspended runnable that is running in a thread that has suspended
 * until is ready to resume
 */
struct SuspendedRunnable {
  using RunnablePtrType = std::unique_ptr<runnable::RunnableNew>;

  /**
   * \brief Construct a new suspended runnable
   *
   * \param[in] in_runnable the runnable
   * \param[in] in_priority the priority to resume with
   */
  SuspendedRunnable(RunnablePtrType in_runnable, PriorityType in_priority)
    : runnable_(std::move(in_runnable)),
      priority_(in_priority)
  { }

  RunnablePtrType runnable_ = nullptr;         /**< the runnable */
  PriorityType priority_ = default_priority;   /**< the resumption priority */
};

} /* end detail namespace */

/**
 * \struct SuspendedUnits
 *
 * \brief Holds \c RunnableNew pointers that have been suspended awaiting a
 * resumption when a dependency clears or the thread is otherwise resumed. Once
 * a runnable is put here, nothing will check it until some other component
 * calls \c resumeRunnable on it with the appropriate thread ID.
 */
struct SuspendedUnits {
  using RunnablePtrType = std::unique_ptr<runnable::RunnableNew>;

  /**
   * \brief Add a suspended runnable that is running in a thread
   *
   * \param[in] tid the thread ID it is running in, now suspended
   * \param[in] runnable the runnable
   * \param[in] p the priority to resume with (optional)
   */
  void addSuspended(
    ThreadIDType tid, RunnablePtrType runnable,
    PriorityType p = default_priority
  );

  /**
   * \brief Resume a thread that is associated with a runnable that is currently
   * suspended
   *
   * \param[in] tid the suspended thread ID
   */
  void resumeRunnable(ThreadIDType tid);

  template <typename SerializerT>
  void serializer(SerializerT& s) {
    s | units_;
  }

private:
  /// A list of suspended runnables that are held here until released to resume
  std::unordered_map<ThreadIDType, detail::SuspendedRunnable> units_;
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_SUSPENDED_UNITS_H*/
