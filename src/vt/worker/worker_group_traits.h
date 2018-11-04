
#if !defined INCLUDED_WORKER_WORKER_GROUP_TRAITS_H
#define INCLUDED_WORKER_WORKER_GROUP_TRAITS_H

#include "vt/config.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#if backend_check_enabled(detector)

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

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_WORKER_WORKER_GROUP_TRAITS_H*/
