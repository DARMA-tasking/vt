/*
//@HEADER
// ************************************************************************
//
//                          memory_pool_equal.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/pool/static_sized/memory_pool_equal.h"
#include "vt/pool/header/pool_header.h"

#include <vector>
#include <cstdint>
#include <cassert>

namespace vt { namespace pool {

template <int64_t num_bytes_t>
MemoryPoolEqual<num_bytes_t>::MemoryPoolEqual(SlotType const in_pool_size)
  : pool_size_(in_pool_size)
{
  resizePool();
}

template <int64_t num_bytes_t>
/*virtual*/ MemoryPoolEqual<num_bytes_t>::~MemoryPoolEqual() {
  debug_print(
    pool, node,
    "cur_slot_={}\n", cur_slot_
  );

  // for (auto i = 0; i < cur_slot_; i++) {
  //   debug_print_force(
  //     pool, node,
  //     "alloc never freed: ptr={}, cur_slot_={}\n", holder_.at(i), i
  //   );
  // }

  for (int i = cur_slot_; i < holder_.size(); i++) {
    free(holder_.at(i));
  }
}

template <int64_t num_bytes_t>
void* MemoryPoolEqual<num_bytes_t>::alloc(
  size_t const& sz, size_t const& oversize
) {
  if (cur_slot_ + 1 >= holder_.size()) {
    resizePool();
  }

  vtAssert(
    cur_slot_+1 < holder_.size(),
    "Must be within pool size, add capability to grow"
  );

  auto const& slot = cur_slot_;
  void* const ptr = holder_[slot];
  void* const ptr_ret = HeaderManagerType::setHeader(
    sz, oversize, static_cast<char*>(ptr)
  );

  debug_print(
    pool, node,
    "alloc: ptr={}, ptr_ret={} cur_slot={}, sz={}, oversize={}\n",
    ptr, ptr_ret, cur_slot_, sz, oversize
  );

  cur_slot_++;

  return ptr_ret;
}

template <int64_t num_bytes_t>
void MemoryPoolEqual<num_bytes_t>::dealloc(void* const t) {
  debug_print(
    pool, node,
    "dealloc t={}, cur_slot={}\n", t, cur_slot_
  );

  vtAssert(
    cur_slot_ - 1 >= 0, "Must be greater than zero"
  );

  auto t_char = static_cast<char*>(t);
  void* const ptr_actual = HeaderManagerType::getHeaderPtr(t_char);

  holder_[--cur_slot_] = ptr_actual;
}

template <int64_t num_bytes_t>
void MemoryPoolEqual<num_bytes_t>::resizePool() {
  SlotType const cur_size = holder_.size();
  SlotType const new_size = cur_size == 0 ? pool_size_ : cur_size * 2;

  holder_.resize(new_size);

  for (auto i = cur_size; i < holder_.size(); i++) {
    holder_[i] = static_cast<void*>(malloc(num_full_bytes_));
  }
}

template <int64_t num_bytes_t>
typename MemoryPoolEqual<num_bytes_t>::SlotType
MemoryPoolEqual<num_bytes_t>::getNumBytes() {
  return num_bytes_;
}

template struct MemoryPoolEqual<memory_size_small>;
template struct MemoryPoolEqual<memory_size_medium>;

}} //end namespace vt::pool
