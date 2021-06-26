/*
//@HEADER
// *****************************************************************************
//
//                        concurrent_deque_locked.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H
#define INCLUDED_VT_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H

#include "vt/config.h"
#include "vt/utils/mutex/mutex.h"
#include "vt/utils/container/concurrent_deque_locked.h"

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

#endif /*INCLUDED_VT_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_IMPL_H*/
