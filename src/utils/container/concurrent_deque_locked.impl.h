
#if ! defined __RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE_LOCKED_IMPL__
#define __RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE_LOCKED_IMPL__

#include "config.h"
#include "concurrent_deque_locked.h"

#include <deque>
#include <cassert>

namespace vt { namespace util { namespace container {

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::emplaceBack(T&& elm) {
  container_mutex_.lock();
  container_.emplace_back(std::forward<T>(elm));
  container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::emplaceFront(T&& elm) {
  container_mutex_.lock();
  container_.emplace_front(std::forward<T>(elm));
  container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::pushBack(T const& elm) {
  container_mutex_.lock();
  container_.push_back(elm);
  container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::pushFront(T const& elm) {
  container_mutex_.lock();
  container_.push_front(elm);
  container_mutex_.unlock();
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TConstRef
ConcurrentDequeLocked<T,LockT>::front() const {
  container_mutex_.lock();
  auto const& val = container_.front();
  container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TConstRef
ConcurrentDequeLocked<T,LockT>::back() const {
  container_mutex_.lock();
  auto const& val = container_.back();
  container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
T ConcurrentDequeLocked<T,LockT>::popGetFront() {
  container_mutex_.lock();
  auto elm = container_.front();
  container_.pop_front();
  container_mutex_.unlock();
  return elm;
}

template <typename T, typename LockT>
T ConcurrentDequeLocked<T,LockT>::popGetBack() {
  container_mutex_.lock();
  auto elm = container_.back();
  container_.pop_back();
  container_mutex_.unlock();
  return elm;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TRef
ConcurrentDequeLocked<T,LockT>::at(SizeType const& pos) {
  container_mutex_.lock();
  auto& val = container_.at(pos);
  container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TConstRef ConcurrentDequeLocked<T,LockT>::at(
  SizeType const& pos
) const {
  container_mutex_.lock();
  auto const& val = container_.at(pos);
  container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TRef ConcurrentDequeLocked<T,LockT>::front() {
  container_mutex_.lock();
  auto& val = container_.front();
  container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TRef ConcurrentDequeLocked<T,LockT>::back() {
  container_mutex_.lock();
  auto& val = container_.back();
  container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::popFront() {
  container_mutex_.lock();
  container_.pop_front();
  container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::popBack() {
  container_mutex_.lock();
  container_.pop_back();
  container_mutex_.unlock();
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::SizeType ConcurrentDequeLocked<T,LockT>::size() {
  container_mutex_.lock();
  auto const& val = container_.size();
  container_mutex_.unlock();
  return val;
}

}}} //end namespace vt::util::container

#endif /*__RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE_LOCKED_IMPL__*/
