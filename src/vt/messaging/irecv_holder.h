/*
//@HEADER
// *****************************************************************************
//
//                                irecv_holder.h
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

#if !defined INCLUDED_VT_MESSAGING_IRECV_HOLDER_H
#define INCLUDED_VT_MESSAGING_IRECV_HOLDER_H

#include "vt/config.h"
#include "vt/configs/arguments/args.h"

#if backend_check_enabled(trace_enabled)
  #include "vt/trace/trace_headers.h"
#endif

#include <vector>

namespace vt { namespace messaging {

/** \file */

/**
 * \struct IRecvHolder
 *
 * \brief Holds a set of pending MPI Irecvs to poll for completion
 */
template <typename T>
struct IRecvHolder {
  using ArgType = vt::arguments::ArgConfig;

  IRecvHolder() = default;

  /**
   * \brief Insert a new element
   *
   * \param[in] u element to insert
   */
  template <typename U>
  void emplace(U&& u) {
    static constexpr std::size_t factor = 4;

    if (holes_.size() * factor > holder_.size()) {
      compress();
    }

    if (holes_.size() > 0) {
      auto const slot = holes_.back();
      holes_.pop_back();
      holder_.emplace(holder_.begin() + slot, std::forward<U>(u));
    } else {
      holder_.emplace_back(std::forward<U>(u));
    }
  }

  /**
   * \brief MPI test all the element in the holder
   *
   * \param[in] c callable to run if the element \c MPI_Test succeeds
   *
   * \return if progress is made
   */
  template <typename Callable>
  bool testAll(Callable c) {
    bool progress_made = false;

    // No active elements, skip any tests
    if (holes_.size() == holder_.size()) {
      return progress_made;
    }

    for (std::size_t i = 0; i < holder_.size(); i++) {
      auto& e = holder_[i];
      if (e.valid) {
        int flag = 0;
        MPI_Status stat;
        MPI_Test(&e.req, &flag, &stat);
        if (flag == 1) {
          #if backend_check_enabled(trace_enabled)
            if (ArgType::vt_trace_mpi) {
              auto tr_note = fmt::format(
                "Irecv completed: from={}", stat.MPI_SOURCE
              );
              trace::addUserNote(tr_note);
            }
          #endif

          c(&e);
          progress_made = true;
          e.valid = false;
          holes_.push_back(i);
        }
      }
    }
    return progress_made;
  }

  /**
   * \brief Compress the holder by removing holes and re-arranging elements
   */
  void compress() {
    std::vector<T> new_holder;
    for (std::size_t i = 0; i < holder_.size(); i++) {
      if (holder_[i].valid) {
        new_holder.emplace_back(std::move(holder_[i]));
      }
    }
    holes_.clear();
    holder_ = std::move(new_holder);
  }

private:
  std::vector<T> holder_;
  std::vector<int> holes_;
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_IRECV_HOLDER_H*/
