
#if !defined __RUNTIME_TRANSPORT_WORKER_TRAITS__
#define __RUNTIME_TRANSPORT_WORKER_TRAITS__

#include "config.h"
#include "worker/worker_common.h"
#include "worker/worker_types.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#if backend_check_enabled(detector)

namespace vt { namespace worker {

template <typename T>
struct WorkerTraits {
  template <typename U>
  using WorkerFunType_t = typename U::WorkerFunType;
  using has_WorkerFunType = detection::is_detected<WorkerFunType_t, T>;

  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using worker_id_t = WorkerIDType const&;
  using has_constructor = detection::is_detected<
    constructor_t, T, worker_id_t, worker_id_t
  >;

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
    // methods: spawn(), join(), dispatch(WorkerFunType), enqueue(WorkUnitType)
    has_spawn::value and has_join::value and
    has_dispatch::value and has_enqueue::value;
};

}} /* end namespace vt::worker */

#endif /*backend_check_enabled(detector)*/

#endif /*__RUNTIME_TRANSPORT_WORKER_TRAITS__*/
