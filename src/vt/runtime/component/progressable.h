/*
//@HEADER
// *****************************************************************************
//
//                                progressable.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_PROGRESSABLE_H
#define INCLUDED_VT_RUNTIME_COMPONENT_PROGRESSABLE_H

namespace vt { namespace runtime { namespace component {

/**
 * \struct Progressable progressable.h vt/runtime/component/progressable.h
 *
 * \brief The abstract \c Progressable trait for pollable components to make
 * progress from the scheduler
 */
struct Progressable {

  virtual ~Progressable() = default;

  /**
   * \brief Progress function for incremental polling
   *
   * \param[in] current_time current time
   *
   * \return the number of units executed---zero if no progress was made
   */
  virtual int progress(TimeType current_time) = 0;

  /**
   * \brief Whether the component needs the current time
   *
   * \return whether it needs time
   */
  virtual bool needsCurrentTime() { return false; }
};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_PROGRESSABLE_H*/
