
#if !defined __RUNTIME_TRANSPORT_MUTEX_TRAITS__
#define __RUNTIME_TRANSPORT_MUTEX_TRAITS__

#include "config.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#if backend_check_enabled(detector)

namespace vt { namespace util { namespace mutex {

template <typename T>
struct MutexTraits {
  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using has_constructor = detection::is_detected<constructor_t, T>;

  template <typename U>
  using copy_constructor_t = decltype(U(std::declval<U const&>()));
  using has_copy_constructor = detection::is_detected<copy_constructor_t, T>;

  template <typename U>
  using lock_t = decltype(std::declval<U>().lock());
  using has_lock = detection::is_detected<lock_t, T>;

  template <typename U>
  using unlock_t = decltype(std::declval<U>().unlock());
  using has_unlock = detection::is_detected<unlock_t, T>;

  template <typename U>
  using try_lock_t = decltype(std::declval<U>().try_lock());
  using has_try_lock = detection::is_detected_exact<bool, try_lock_t, T>;

  // This defines what it means to be an `mutex'
  static constexpr auto const is_mutex =
    // default constructor and copy constructor
    has_constructor::value and not has_copy_constructor::value and
    // methods: lock(), unlock(), try_lock()
    has_lock::value and has_unlock::value and
    has_try_lock::value;
};

}}} /* end namespace vt::util::mutex */

#endif /*backend_check_enabled(detector)*/

#endif /*__RUNTIME_TRANSPORT_MUTEX_TRAITS__*/
