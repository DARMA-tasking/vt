/*
//@HEADER
// *****************************************************************************
//
//                               request_holder.h
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

#if !defined INCLUDED_VT_MESSAGING_REQUEST_HOLDER_H
#define INCLUDED_VT_MESSAGING_REQUEST_HOLDER_H

#include "vt/config.h"
#include "vt/configs/arguments/app_config.h"

// Unfortunate header leak for VT_ALLOW_MPI_CALLS
#include "vt/runtime/mpi_access.h"

#if vt_check_enabled(trace_enabled)
  #include "vt/trace/trace_headers.h"
#endif

#include <vector>

namespace vt { namespace messaging {

/** \file */

/**
 * \struct RequestHolder
 *
 * \brief Holds a set of pending MPI Irecvs to poll for completion
 */
template <typename T>
struct RequestHolder {

# if vt_check_enabled(trace_enabled)
  explicit RequestHolder(trace::UserEventIDType in_trace_user_event)
    : trace_user_event_(in_trace_user_event)
  { }
# else
  RequestHolder() = default;
#endif

  /**
   * \brief Insert a new element
   *
   * \param[in] u element to insert
   */
  template <typename U>
  void emplace(U&& u) {
    holder_.emplace_back(std::forward<U>(u));
  }

  /**
   * \brief MPI test all the element in the holder
   *
   * \param[in] c callable to run if the element \c MPI_Test succeeds
   * \param[out] num_mpi_tests number of MPI tests that the holder performed
   *
   * \return if progress is made
   */
  template <typename Callable>
  bool testAll(Callable c, int& num_mpi_tests) {
#   if vt_check_enabled(trace_enabled)
    std::size_t const holder_size_start = holder_.size();
    auto tr_begin = TimeType{};
    if (theConfig()->vt_trace_irecv_polling) {
      tr_begin = vt::timing::getCurrentTime();
    }
#   endif

    bool progress_made = false;

    for (std::size_t i = 0; i < holder_.size(); ) {
      auto& e = holder_[i];
      vtAssert(e.valid, "Must be valid");

      auto done = e.test(num_mpi_tests);

      if (not done) {
        ++i;
        continue;
      }

      c(&e);
      progress_made = true;
      e.valid = false;

      if (i < holder_.size() - 1) {
        holder_[i] = std::move(holder_.back());
      }

      holder_.pop_back();
    }

#   if vt_check_enabled(trace_enabled)
    if (theConfig()->vt_trace_irecv_polling) {
       if (holder_size_start > 0) {
         auto tr_end = vt::timing::getCurrentTime();
         auto tr_note = fmt::format(
           "completed {} of {}",
           holder_size_start - holder_.size(),
           holder_size_start
         );
         trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_user_event_);
       }
    }
#   endif

    return progress_made;
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | holder_;
  # if vt_check_enabled(trace_enabled)
    s | trace_user_event_;
  # endif
  }

private:
  std::vector<T> holder_;

# if vt_check_enabled(trace_enabled)
  trace::UserEventIDType trace_user_event_ = trace::no_user_event_id;
# endif
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_REQUEST_HOLDER_H*/
