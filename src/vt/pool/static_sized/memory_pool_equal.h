/*
//@HEADER
// *****************************************************************************
//
//                             memory_pool_equal.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_POOL_STATIC_SIZED_MEMORY_POOL_EQUAL_H
#define INCLUDED_POOL_STATIC_SIZED_MEMORY_POOL_EQUAL_H

#include "vt/config.h"
#include "vt/messaging/envelope.h"
#include "vt/context/context.h"
#include "vt/pool/header/pool_header.h"

#include <vector>
#include <cstdint>

namespace vt { namespace pool {

static constexpr size_t const small_msg_size_buf =
  sizeof(int64_t)*8 - sizeof(EpochTagEnvelope);
static constexpr size_t const memory_size_small =
  sizeof(EpochTagEnvelope) + small_msg_size_buf;

static constexpr size_t const medium_msg_size_buf =
  sizeof(int64_t)*128 - sizeof(EpochTagEnvelope);
static constexpr size_t const memory_size_medium =
  sizeof(EpochTagEnvelope) + medium_msg_size_buf;

template <int64_t num_bytes_t>
struct MemoryPoolEqual {
  using ContainerType = std::vector<void*>;
  using SlotType = int64_t;
  using HeaderType = Header;
  using HeaderManagerType = HeaderManager;

  static constexpr SlotType const fst_pool_slot = 0;
  static constexpr SlotType const default_pool_size = 1024;

  MemoryPoolEqual(SlotType const in_pool_size = default_pool_size);

  virtual ~MemoryPoolEqual();

  void* alloc(size_t const& sz, size_t const& oversize);
  void dealloc(void* const t);
  void resizePool();
  SlotType getNumBytes();

private:
  SlotType const num_bytes_ = num_bytes_t;
  SlotType const num_full_bytes_ = num_bytes_t + sizeof(Header);

  SlotType pool_size_ = default_pool_size;
  SlotType cur_slot_ = fst_pool_slot;

  ContainerType holder_;
};

}} //end namespace vt::pool

#endif /*INCLUDED_POOL_STATIC_SIZED_MEMORY_POOL_EQUAL_H*/
