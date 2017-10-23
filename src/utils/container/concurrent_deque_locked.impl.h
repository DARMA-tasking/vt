
#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H

#include "config.h"
#include "concurrent_deque_locked.h"

#include <deque>
#include <cassert>

namespace vt { namespace util { namespace container {

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::emplaceBack(T&& elm) {
  if (needs_lock_) container_mutex_.lock();
  container_.emplace_back(std::forward<T>(elm));
  if (needs_lock_) container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::emplaceFront(T&& elm) {
  if (needs_lock_) container_mutex_.lock();
  container_.emplace_front(std::forward<T>(elm));
  if (needs_lock_) container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::pushBack(T const& elm) {
  if (needs_lock_) container_mutex_.lock();
  container_.push_back(elm);
  if (needs_lock_) container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::pushFront(T const& elm) {
  if (needs_lock_) container_mutex_.lock();
  container_.push_front(elm);
  if (needs_lock_) container_mutex_.unlock();
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TConstRef
ConcurrentDequeLocked<T,LockT>::front() const {
  if (needs_lock_) container_mutex_.lock();
  auto const& val = container_.front();
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TConstRef
ConcurrentDequeLocked<T,LockT>::back() const {
  if (needs_lock_) container_mutex_.lock();
  auto const& val = container_.back();
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
T ConcurrentDequeLocked<T,LockT>::popGetFront() {
  if (needs_lock_) container_mutex_.lock();
  auto elm = container_.front();
  container_.pop_front();
  if (needs_lock_) container_mutex_.unlock();
  return elm;
}

template <typename T, typename LockT>
T ConcurrentDequeLocked<T,LockT>::popGetBack() {
  if (needs_lock_) container_mutex_.lock();
  auto elm = container_.back();
  container_.pop_back();
  if (needs_lock_) container_mutex_.unlock();
  return elm;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TRef
ConcurrentDequeLocked<T,LockT>::at(SizeType const& pos) {
  if (needs_lock_) container_mutex_.lock();
  auto& val = container_.at(pos);
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TConstRef ConcurrentDequeLocked<T,LockT>::at(
  SizeType const& pos
) const {
  if (needs_lock_) container_mutex_.lock();
  auto const& val = container_.at(pos);
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TRef ConcurrentDequeLocked<T,LockT>::front() {
  if (needs_lock_) container_mutex_.lock();
  auto& val = container_.front();
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::TRef ConcurrentDequeLocked<T,LockT>::back() {
  if (needs_lock_) container_mutex_.lock();
  auto& val = container_.back();
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::popFront() {
  if (needs_lock_) container_mutex_.lock();
  container_.pop_front();
  if (needs_lock_) container_mutex_.unlock();
}

template <typename T, typename LockT>
void ConcurrentDequeLocked<T,LockT>::popBack() {
  if (needs_lock_) container_mutex_.lock();
  container_.pop_back();
  if (needs_lock_) container_mutex_.unlock();
}

template <typename T, typename LockT>
typename ConcurrentDequeLocked<T,LockT>::SizeType ConcurrentDequeLocked<T,LockT>::size() {
  if (needs_lock_) container_mutex_.lock();
  auto const& val = container_.size();
  if (needs_lock_) container_mutex_.unlock();
  return val;
}

}}} //end namespace vt::util::container

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H*/
