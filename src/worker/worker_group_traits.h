
#if !defined __RUNTIME_TRANSPORT_WORKER_GROUP_TRAITS__
#define __RUNTIME_TRANSPORT_WORKER_GROUP_TRAITS__

#include "config.h"
#include "worker/worker_common.h"
#include "worker/worker_types.h"

#if backend_check_enabled(detector)
  #include "utils/detector/detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#if backend_check_enabled(detector)

namespace vt { namespace worker {

using namespace vt::util;

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
  using joinWorkers_t = decltype(std::declval<U>().joinWorkers());
  using has_joinWorkers = detection::is_detected<joinWorkers_t, T>;

  using worker_id_t = WorkerIDType const&;
  using work_unit_t = WorkUnitType const&;

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

  // This defines what it means to be an `Worker'
  static constexpr auto const is_worker =
    // default constructor and copy constructor
    has_constructor::value and has_default_constructor::value and
    // using WorkerType
    has_WorkerType::value and
    // methods: spawnWorkers, joinWorkers, enqueueAnyWorker,
    //          enqueueForWorker, enqueueAllWorkers
    has_spawnWorkers::value and has_joinWorkers::value and
    has_enqueueAnyWorker::value and has_enqueueForWorker::value and
    has_enqueueAllWorkers::value;
};

}} /* end namespace vt::worker */

#endif /*backend_check_enabled(detector)*/

#endif /*__RUNTIME_TRANSPORT_WORKER_GROUP_TRAITS__*/
