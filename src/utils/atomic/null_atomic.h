
#if !defined INCLUDED_UTILS_ATOMIC_NULL_ATOMIC_H
#define INCLUDED_UTILS_ATOMIC_NULL_ATOMIC_H

#include "config.h"

#if backend_no_threading

// Needed for memory order
#include <atomic>

namespace vt { namespace util { namespace atomic {

static constexpr std::memory_order order_default = std::memory_order_seq_cst;

template <typename T>
struct AtomicNull {
  using OrderType = std::memory_order;

  AtomicNull(T&& in_value) : value_(std::forward<T>(in_value)) { }
  AtomicNull(T const& in_value) : value_(in_value) { }
  AtomicNull(AtomicNull const&) = delete;
  AtomicNull& operator=(AtomicNull const&) = delete;

  inline void add(T const in) { value_ += in; }
  inline void sub(T const in) { value_ -= in; }
  inline T fetch_add(T const in) { T tmp = value_; value_ += in; return tmp; }
  inline T add_fetch(T const in) { value_ += in; return value_; }
  inline T fetch_sub(T const in) { T tmp = value_; value_ -= in; return tmp; }
  inline T sub_fetch(T const in) { value_ -= in; return value_; }

  // pre-increment
  T operator++() { return fetch_add(1) + 1; }
  T operator--() { return fetch_sub(1) - 1; }
  // post-increment
  T operator++(int) { return fetch_add(1); }
  T operator--(int) { return fetch_sub(1); }
  // operators +=, -= (pre-increment type operation)
  T operator+=(T const& val) { return fetch_add(val) + val; }
  T operator-=(T const& val) { return fetch_sub(val) + val; }

  operator T() const { return load(); }
  inline T load(OrderType t = order_default) const { return value_; }
  inline void store(T in, OrderType t = order_default) { value_ = in; }
  T operator=(T in) { store(in); return in; }

  bool compare_exchange_strong(
    T& expected, T desired,
    OrderType st = order_default, OrderType sf = order_default
  ) {
    return value_ == expected ? value_ = desired, true : false;
  }

private:
  T value_;
};

}}} /* end namespace vt::util::atomic */

#endif /*backend_no_threading*/

#endif /*INCLUDED_UTILS_ATOMIC_NULL_ATOMIC_H*/
