/*
//@HEADER
// *****************************************************************************
//
//                            worker_group_traits.h
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

#if !defined INCLUDED_VT_WORKER_WORKER_GROUP_TRAITS_H
#define INCLUDED_VT_WORKER_WORKER_GROUP_TRAITS_H

#include "vt/config.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"

#include "detector_headers.h"

namespace vt { namespace worker {

template <typename T>
struct WorkerGroupTraits {
  template <typename U>
  using WorkerType_t = typename U::WorkerType;
  using has_WorkerType = detection::is_detected<WorkerType_t, T>;

  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using worker_cnt_t = WorkerCountType const&;
  using has_constructor = detection::is_detected<constructor_t, T, worker_cnt_t>;
  using has_default_constructor = detection::is_detected<constructor_t, T>;

  template <typename U>
  using spawnWorkers_t = decltype(std::declval<U>().spawnWorkers());
  using has_spawnWorkers = detection::is_detected<spawnWorkers_t, T>;

  template <typename U>
  using spawnWorkersBlock_t = decltype(std::declval<U>().spawnWorkersBlock(
                                         std::declval<WorkerCommFnType>()
                                       ));
  using has_spawnWorkersBlock = detection::is_detected<spawnWorkersBlock_t, T>;

  template <typename U>
  using joinWorkers_t = decltype(std::declval<U>().joinWorkers());
  using has_joinWorkers = detection::is_detected<joinWorkers_t, T>;

  template <typename U>
  using progress_t = decltype(std::declval<U>().progress());
  using has_progress = detection::is_detected<progress_t, T>;

  using worker_id_t = WorkerIDType const&;
  using work_unit_t = WorkUnitType const&;
  using work_unit_cnt_t = WorkUnitCountType;

  template <typename U>
  using finished_t = decltype(std::declval<U>().finished(
                                std::declval<worker_id_t>(),
                                std::declval<work_unit_cnt_t>()
                              ));
  using has_finished = detection::is_detected<finished_t, T>;

  template <typename U>
  using enqueueAnyWorker_t = decltype(std::declval<U>().enqueueAnyWorker(
                                        std::declval<work_unit_t>())
                                     );
  using has_enqueueAnyWorker = detection::is_detected<enqueueAnyWorker_t, T>;

  template <typename U>
  using enqueueForWorker_t = decltype(std::declval<U>().enqueueForWorker(
                                        std::declval<worker_id_t>(),
                                        std::declval<work_unit_t>()
                                      ));
  using has_enqueueForWorker = detection::is_detected<enqueueForWorker_t, T>;

  template <typename U>
  using enqueueAllWorkers_t = decltype(std::declval<U>().enqueueAllWorkers(
                                         std::declval<work_unit_t>()
                                       ));
  using has_enqueueAllWorkers = detection::is_detected<enqueueAllWorkers_t, T>;

  template <typename U>
  using enqueueCommThread_t = decltype(std::declval<U>().enqueueCommThread(
                                        std::declval<work_unit_t>())
                                     );
  using has_enqueueCommThread = detection::is_detected<enqueueCommThread_t, T>;

  template <typename U>
  using commScheduler_t = decltype(std::declval<U>().commScheduler());
  using has_commScheduler = detection::is_detected_convertible<
    bool, commScheduler_t, T
  >;

  // This defines what it means to be an `Worker'
  static constexpr auto const is_worker =
    // default constructor and copy constructor
    has_constructor::value and has_default_constructor::value and
    // using WorkerType
    has_WorkerType::value and
    // methods: spawnWorkers, spawnWorkersBlock, joinWorkers, enqueueAnyWorker,
    //          enqueueForWorker, enqueueAllWorkers, enqueueCommThread
    has_spawnWorkers::value and has_spawnWorkersBlock::value and
    has_joinWorkers::value and has_enqueueAnyWorker::value and
    has_enqueueForWorker::value and has_enqueueAllWorkers::value and
    has_progress::value and has_finished::value and has_commScheduler::value and
    has_enqueueCommThread::value;
};

}} /* end namespace vt::worker */

#endif /*INCLUDED_VT_WORKER_WORKER_GROUP_TRAITS_H*/
