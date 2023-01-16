/*
//@HEADER
// *****************************************************************************
//
//                                   pool.cc
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

#include "vt/config.h"
#include "vt/pool/pool.h"
#include "vt/pool/static_sized/memory_pool_equal.h"

#include <cstdlib>
#include <cstdint>
#include <cassert>

namespace vt { namespace pool {

Pool::Pool()
  : small_msg(initSPool()), medium_msg(initMPool())
{ }

/*static*/Pool::MemPoolSType Pool::initSPool() {
  return std::make_unique<MemoryPoolType<memory_size_small>>();
}

/*static*/ Pool::MemPoolMType Pool::initMPool() {
  return std::make_unique<MemoryPoolType<memory_size_medium>>(64);
}

Pool::ePoolSize Pool::getPoolType(
  size_t const& num_bytes, size_t const& oversize
) const {
  // Header size is accounted for internally in MemoryPoolEqual
  // Blocks will include header size, but getNumBytes won't so we shouldn't count
  // it here
  auto const& total_bytes = num_bytes + oversize;
  if (total_bytes <= static_cast<size_t>(small_msg->getNumBytes())) {
    return ePoolSize::Small;
  } else if (total_bytes <= static_cast<size_t>(medium_msg->getNumBytes())) {
    return ePoolSize::Medium;
  } else {
    return ePoolSize::Malloc;
  }
}

void* Pool::tryPooledAlloc(size_t const& num_bytes, size_t const& oversize) {
  ePoolSize const pool_type = getPoolType(num_bytes, oversize);

  if (pool_type != ePoolSize::Malloc) {
    return pooledAlloc(num_bytes, oversize, pool_type);
  } else {
    return nullptr;
  }
}

bool Pool::tryPooledDealloc(void* const buf) {
  auto buf_char = static_cast<char*>(buf);
  auto const& actual_alloc_size = HeaderManagerType::getHeaderBytes(buf_char);
  auto const& oversize = HeaderManagerType::getHeaderOversizeBytes(buf_char);
  ePoolSize const pool_type = getPoolType(actual_alloc_size, oversize);

  if (pool_type != ePoolSize::Malloc) {
    poolDealloc(buf, pool_type);
    return true;
  } else {
    return false;
  }
}

void* Pool::pooledAlloc(
  size_t const& num_bytes, size_t const& oversize, ePoolSize const pool_type
) {
  void* ret = nullptr;

  vt_debug_print(
    normal, pool,
    "Pool::pooled_alloc of size={}, type={}, ret={}\n",
    num_bytes, print_pool_type(pool_type), ret
  );

  if (pool_type == ePoolSize::Small) {
    auto pool = small_msg.get();
    ret = pool->alloc(num_bytes, oversize);
  } else if (pool_type == ePoolSize::Medium) {
    auto pool = medium_msg.get();
    ret = pool->alloc(num_bytes, oversize);
  } else {
    vtAssert(0, "Pool must be valid");
    ret = nullptr;
  }

  return ret;
}

void Pool::poolDealloc(void* const buf, ePoolSize const pool_type) {
  vt_debug_print(
    normal, pool,
    "Pool::pooled_dealloc of ptr={}, type={}\n",
    print_ptr(buf), print_pool_type(pool_type)
  );

  if (pool_type == ePoolSize::Small) {
    small_msg->dealloc(buf);
  } else if (pool_type == ePoolSize::Medium) {
    medium_msg->dealloc(buf);
  } else {
    vtAssert(0, "Pool must be valid");
  }
}

void* Pool::defaultAlloc(size_t const& num_bytes, size_t const& oversize) {
  auto alloc_buf = std::malloc(num_bytes + oversize + sizeof(HeaderType));
  return HeaderManagerType::setHeader(
    num_bytes, oversize, static_cast<char*>(alloc_buf)
  );
}

void Pool::defaultDealloc(void* const ptr) {
  std::free(ptr);
}

void* Pool::alloc(size_t const& num_bytes, size_t oversize) {
  void* ret = nullptr;

  #if vt_check_enabled(memory_pool)
    ret = tryPooledAlloc(num_bytes, oversize);
  #endif

  // Fall back to the default allocated if the pooled allocated fails to return
  // a valid pointer
  if (ret == nullptr) {
    ret = defaultAlloc(num_bytes, oversize);
  }

  vt_debug_print(
    normal, pool,
    "Pool::alloc of size={}, ret={}\n",
    num_bytes, ret
  );

  return ret;
}

void Pool::dealloc(void* const buf) {
  auto buf_char = static_cast<char*>(buf);
  auto const& actual_alloc_size = HeaderManagerType::getHeaderBytes(buf_char);
  auto const& ptr_actual = HeaderManagerType::getHeaderPtr(buf_char);

  vt_debug_print(
    normal, pool,
    "Pool::dealloc of buf={}, alloc_size={}, ptr={}\n",
    buf, actual_alloc_size, print_ptr(ptr_actual)
  );

  bool success = false;

  #if vt_check_enabled(memory_pool)
    success = tryPooledDealloc(buf);
  #endif

  if (!success) {
    defaultDealloc(ptr_actual);
  }
}

Pool::SizeType Pool::remainingSize(void* const buf) const {
  #if vt_check_enabled(memory_pool)
    auto buf_char = static_cast<char*>(buf);
    auto const& actual_alloc_size = HeaderManagerType::getHeaderBytes(buf_char);
    auto const& oversize = HeaderManagerType::getHeaderOversizeBytes(buf_char);

    ePoolSize const pool_type = getPoolType(actual_alloc_size, oversize);

    if (pool_type == ePoolSize::Small) {
      return small_msg->getNumBytes() - actual_alloc_size;
    } else if (pool_type == ePoolSize::Medium) {
      return medium_msg->getNumBytes() - actual_alloc_size;
    } else {
      return oversize;
    }
  #else
    return 0;
  #endif
}

Pool::SizeType Pool::allocatedSize(void* const buf) const {
  auto buf_char = static_cast<char*>(buf);
  return HeaderManagerType::getHeaderBytes(buf_char) + HeaderManagerType::getHeaderOversizeBytes(buf_char);
}

bool
Pool::tryGrowAllocation(void* buf, size_t grow_amount) {
  // For non-pooled alloc, this condition will always be true
  // since remainingSize(buf) would be 0
  if ( remainingSize(buf) < grow_amount )
    return false;

  auto *header = reinterpret_cast<Header*>(HeaderManagerType::getHeaderPtr(reinterpret_cast<char*>(buf)));
  header->alloc_size += grow_amount;
  return true;
}

bool Pool::active() const {
  return vt_check_enabled(memory_pool);
}

 bool Pool::active_env() const {
  return vt_check_enabled(memory_pool) &&
    !vt_check_enabled(no_pool_alloc_env);
}

}} //end namespace vt::pool
