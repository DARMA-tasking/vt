
#if !defined INCLUDED_UTILS_ATOMIC_OMP_ATOMIC_H
#define INCLUDED_UTILS_ATOMIC_OMP_ATOMIC_H

#include "config.h"

#if backend_check_enabled(openmp)

#include <atomic>
#include <omp.h>

namespace vt { namespace util { namespace atomic {

template <typename T>
struct AtomicOMP {
  AtomicOMP(T&& in_value) : value_(std::forward<T>(in_value)) { }
  AtomicOMP(T const& in_value) : value_(in_value) { }
  AtomicOMP(AtomicOMP const&) = delete;
  AtomicOMP& operator=(AtomicOMP const&) = delete;

  void add(T const in) {
    #pragma omp atomic update
    value_ += in;
  }

  void sub(T const in) {
    #pragma omp atomic update
    value_ -= in;
  }

  T fetch_add(T const in) {
    T tmp;
    #pragma omp atomic capture
    {
      tmp = value_;
      value_ += in;
    }
    return tmp;
  }

  T add_fetch(T const in) {
    T tmp;
    #pragma omp atomic capture
    {
      value_ += in;
      tmp = value_;
    }
    return tmp;
  }

  T fetch_sub(T const in) {
    T tmp;
    #pragma omp atomic capture
    {
      tmp = value_;
      value_ -= in;
    }
    return tmp;
  }

  T sub_fetch(T const in) {
    T tmp;
    #pragma omp atomic capture
    {
      value_ -= in;
      tmp = value_;
    }
    return tmp;
  }

  // pre-increment
  T operator++() { return fetch_add(1) + 1; }
  T operator--() { return fetch_sub(1) - 1; }
  // post-increment
  T operator++(int) { return fetch_add(1); }
  T operator--(int) { return fetch_sub(1); }
  // operators +=, -= (pre-increment type operation)
  T operator+=(T const& val) { return fetch_add(val) + val; }
  T operator-=(T const& val) { return fetch_sub(val) + val; }

  operator T() const {
    return load();
  }

  T load(std::memory_order order = std::memory_order_seq_cst) const {
    T tmp;
    #pragma omp atomic read
    tmp = value_;
    return tmp;
  }

  void store(T in, std::memory_order order = std::memory_order_seq_cst) {
    #pragma omp atomic write
    value_ = in;
  }

  T operator=(T in) {
    store(in);
    return in;
  }

  bool compare_exchange_strong(
    T& expected, T desired,
    std::memory_order success = std::memory_order_seq_cst,
    std::memory_order failure = std::memory_order_seq_cst
  ) {
    bool cas_succeed = false;
    #pragma omp critical
    {
      if (value_ == expected) {
        value_ = desired;
        cas_succeed = true;
      }
    }
    return cas_succeed;
  }

private:
  T value_;
};

}}} /* end namespace vt::util::atomic */

#endif

#endif /*INCLUDED_UTILS_ATOMIC_OMP_ATOMIC_H*/
