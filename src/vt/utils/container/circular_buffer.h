/*
//@HEADER
// *****************************************************************************
//
//                              circular_buffer.h
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

#if !defined INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_BUFFER_H
#define INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_BUFFER_H

#include "vt/config.h"

namespace vt { namespace util { namespace container {

template <typename T, int size>
struct CircularBuffer {

  CircularBuffer() = default;

private:
  int getNextEntry() const {
    int next_entry = head_ + 1;
    if (next_entry == size) {
      next_entry = 0;
    }
    return next_entry;
  }

public:
  template <typename... Args>
  void emplace(Args&&... args) {
    int const next_entry = getNextEntry();
    elms_[head_] = T{std::forward<Args>(args)...};
    head_ = next_entry;
  }

  void push(T&& t) {
    int const next_entry = getNextEntry();
    elms_[head_] = std::move(t);
    head_ = next_entry;
  }

  void push(T const& t) {
    int const next_entry = getNextEntry();
    elms_[head_] = t;
    head_ = next_entry;
  }

  T pop() {
    T elm = std::move(elms_[tail_]);
    ++tail_;
    if (tail_ == size) {
      tail_ = 0;
    }
    return elm;
  }

  int numFree() const {
    if (head_ == tail_) {
      return size - 1;
    } else if (head_ < tail_) {
      return tail_ - head_ - 1;
    } else {
      return size + tail_ - head_ - 1;
    }
  }

  bool empty() const { return head_ == tail_; }
  bool full() const { return numFree() == 0; }
  int len() const { return size - 1 - numFree(); }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | head_;
    s | tail_;
    s | elms_; // this is inefficient, but it's use is footprinting
  }

private:
  int head_ = 0;
  int tail_ = 0;
  std::array<T, size> elms_;
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_BUFFER_H*/
