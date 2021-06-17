/*
//@HEADER
// *****************************************************************************
//
//                               worker_traits.h
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

#if !defined INCLUDED_WORKER_WORKER_TRAITS_H
#define INCLUDED_WORKER_WORKER_TRAITS_H

#include "vt/config.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"

#include "detector_headers.h"

namespace vt { namespace worker {

template <typename T>
struct WorkerTraits {
  template <typename U>
  using WorkerFunType_t = typename U::WorkerFunType;
  using has_WorkerFunType = detection::is_detected<WorkerFunType_t, T>;

  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using worker_id_t = WorkerIDType const&;
  using worker_count_t = WorkerCountType const&;
  using worker_finished_fn_t = WorkerFinishedFnType;
  using has_constructor = detection::is_detected<
    constructor_t, T, worker_id_t, worker_count_t, worker_finished_fn_t
  >;

  template <typename U>
  using progress_t = decltype(std::declval<U>().progress());
  using has_progress = detection::is_detected<progress_t, T>;

  template <typename U>
  using copy_constructor_t = decltype(U(std::declval<U const&>()));
  using has_copy_constructor = detection::is_detected<copy_constructor_t, T>;

  template <typename U>
  using spawn_t = decltype(std::declval<U>().spawn());
  using has_spawn = detection::is_detected<spawn_t, T>;

  template <typename U>
  using join_t = decltype(std::declval<U>().join());
  using has_join = detection::is_detected<join_t, T>;

  template <typename U>
  using dispatch_t = decltype(std::declval<U>().dispatch(
                                std::declval<typename U::WorkerFunType>())
                             );
  using has_dispatch = detection::is_detected<dispatch_t, T>;

  template <typename U>
  using enqueue_t = decltype(std::declval<U>().enqueue(
                                std::declval<WorkUnitType>())
                             );
  using has_enqueue = detection::is_detected<enqueue_t, T>;

  // This defines what it means to be an `Worker'
  static constexpr auto const is_worker =
    // default constructor and copy constructor
    has_constructor::value and not has_copy_constructor::value and
    // using WorkerFunType
    has_WorkerFunType::value and
    // methods: spawn(), join(), dispatch(WorkerFunType), enqueue(WorkUnitType),
    //          progress()
    has_spawn::value and has_join::value and has_progress::value and
    has_dispatch::value and has_enqueue::value;
};

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_TRAITS_H*/
