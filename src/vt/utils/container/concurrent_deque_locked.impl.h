
#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H

#include "config.h"
#include "utils/mutex/mutex.h"
#include "concurrent_deque_locked.h"

#include <deque>
#include <cassert>

namespace vt { namespace util { namespace container {

using ::vt::util::mutex::LockGuardPtrType;
using ::vt::util::mutex::MutexType;

template <typename T>
MutexType* ConcurrentDequeLocked<T>::getMutex() {
  return needs_lock_ ? &container_mutex_: nullptr;
}

template <typename T>
void ConcurrentDequeLocked<T>::emplaceBack(T&& elm) {
  LockGuardPtrType lock(getMutex());
  container_.emplace_back(std::forward<T>(elm));
}

template <typename T>
void ConcurrentDequeLocked<T>::emplaceFront(T&& elm) {
  LockGuardPtrType lock(getMutex());
  container_.emplace_front(std::forward<T>(elm));
}

template <typename T>
void ConcurrentDequeLocked<T>::pushBack(T const& elm) {
  LockGuardPtrType lock(getMutex());
  container_.push_back(elm);
}

template <typename T>
void ConcurrentDequeLocked<T>::pushFront(T const& elm) {
  LockGuardPtrType lock(getMutex());
  container_.push_front(elm);
}

template <typename T>
typename ConcurrentDequeLocked<T>::TConstRef
ConcurrentDequeLocked<T>::front() const {
  LockGuardPtrType lock(getMutex());
  auto const& val = container_.front();
  return val;
}

template <typename T>
typename ConcurrentDequeLocked<T>::TConstRef
ConcurrentDequeLocked<T>::back() const {
  LockGuardPtrType lock(getMutex());
  auto const& val = container_.back();
  return val;
}

template <typename T>
T ConcurrentDequeLocked<T>::popGetFront() {
  LockGuardPtrType lock(getMutex());
  auto elm = container_.front();
  container_.pop_front();
  return elm;
}

template <typename T>
T ConcurrentDequeLocked<T>::popGetBack() {
  LockGuardPtrType lock(getMutex());
  auto elm = container_.back();
  container_.pop_back();
  return elm;
}

template <typename T>
typename ConcurrentDequeLocked<T>::TRef
ConcurrentDequeLocked<T>::at(SizeType const& pos) {
  LockGuardPtrType lock(getMutex());
  auto& val = container_.at(pos);
  return val;
}

template <typename T>
typename ConcurrentDequeLocked<T>::TConstRef ConcurrentDequeLocked<T>::at(
  SizeType const& pos
) const {
  LockGuardPtrType lock(getMutex());
  auto const& val = container_.at(pos);
  return val;
}

template <typename T>
typename ConcurrentDequeLocked<T>::TRef ConcurrentDequeLocked<T>::front() {
  LockGuardPtrType lock(getMutex());
  auto& val = container_.front();
  return val;
}

template <typename T>
typename ConcurrentDequeLocked<T>::TRef ConcurrentDequeLocked<T>::back() {
  LockGuardPtrType lock(getMutex());
  auto& val = container_.back();
  return val;
}

template <typename T>
void ConcurrentDequeLocked<T>::popFront() {
  LockGuardPtrType lock(getMutex());
  container_.pop_front();
}

template <typename T>
void ConcurrentDequeLocked<T>::popBack() {
  LockGuardPtrType lock(getMutex());
  container_.pop_back();
}

template <typename T>
typename ConcurrentDequeLocked<T>::SizeType ConcurrentDequeLocked<T>::size() {
  LockGuardPtrType lock(getMutex());
  auto const& val = container_.size();
  return val;
}

}}} //end namespace vt::util::container

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H*/
