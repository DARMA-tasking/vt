/*
//@HEADER
// *****************************************************************************
//
//                                   queue.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_SCHEDULER_QUEUE_H
#define INCLUDED_VT_SCHEDULER_QUEUE_H

#include "vt/config.h"
#include "vt/utils/container/circular_buffer.h"

#include <queue>

namespace vt { namespace sched {

template <typename T>
struct Queue {
  Queue() = default;
  Queue(Queue const&) = default;
  Queue(Queue&&) = default;

  void push(T elm) {
    if (buf_.full()) {
      impl_.push(elm);
    } else {
      buf_.push(elm);
    }
  }

  void emplace(T&& elm) {
    if (buf_.full()) {
      impl_.emplace(std::move(elm));
    } else {
      buf_.push(std::move(elm));
    }
  }

  T pop() {
    vtAssert(not buf_.empty(), "Must have at least one element");
    auto t = buf_.pop();
    if (not impl_.empty()) {
      buf_.push(std::move(impl_.front()));
      impl_.pop();
    }
    return t;
  }

  std::size_t size() const {
    return buf_.size() + impl_.size();
  }

  bool empty() const {
    vtAssert(not buf_.empty() or impl_.empty(), "Buf empty implies queue empty");
    return buf_.empty();
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | impl_;
    s | buf_;
  }

private:
  util::container::CircularBuffer<T, 64> buf_;
  std::queue<T, std::deque<T>> impl_;
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_QUEUE_H*/
