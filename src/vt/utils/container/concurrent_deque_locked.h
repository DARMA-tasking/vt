/*
//@HEADER
// *****************************************************************************
//
//                          concurrent_deque_locked.h
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

#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/utils/mutex/mutex.h"

#include <deque>
#include <mutex>
#include <functional>
#include <cassert>

namespace vt { namespace util { namespace container {

using ::vt::util::mutex::MutexType;

/*
 * Implement a very simple concurrent deque that just uses std::mutex for access
 * control... obviously this should be greatly improved and made lock free but
 * this enables programming to a consistent interface
 */

template <typename T>
struct ConcurrentDequeLocked {
  using ContainerType = std::deque<T>;
  using SizeType = typename ContainerType::size_type;
  using TConstRef = typename ContainerType::const_reference;
  using TRef = typename ContainerType::reference;

  ConcurrentDequeLocked()
    : ConcurrentDequeLocked(theContext() ? theContext()->hasWorkers() : true)
  { }
  explicit ConcurrentDequeLocked(bool in_needs_lock)
    : needs_lock_(in_needs_lock)
  { }
  ConcurrentDequeLocked(ConcurrentDequeLocked const&) = delete;

  virtual ~ConcurrentDequeLocked() { }

  void emplaceBack(T&& elm);
  void emplaceFront(T&& elm);
  void pushBack(T const& elm);
  void pushFront(T const& elm);

  TConstRef front() const;
  TConstRef back() const;

  T popGetFront();
  T popGetBack();

  TRef at(SizeType const& pos);
  TConstRef at(SizeType const& pos) const;

  TRef front();
  TRef back();

  void popFront();
  void popBack();

  SizeType size();

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | needs_lock_
      | container_
      | container_mutex_;
  }

private:
  MutexType* getMutex();

private:
  bool needs_lock_ = true;
  MutexType container_mutex_{};
  ContainerType container_;
};

}}} //end namespace vt::util::container

#include "vt/utils/container/concurrent_deque_locked.impl.h"

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_LOCKED_H*/

