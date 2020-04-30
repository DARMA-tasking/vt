/*
//@HEADER
// *****************************************************************************
//
//                                    pool.h
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

#if !defined INCLUDED_POOL_POOL_H
#define INCLUDED_POOL_POOL_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/pool/static_sized/memory_pool_equal.h"
#include "vt/pool/header/pool_header.h"

#include <vector>
#include <cstdint>
#include <cassert>
#include <memory>

namespace vt { namespace pool {

struct Pool : runtime::component::Component<Pool> {
  using SizeType = size_t;
  using HeaderType = Header;
  using HeaderManagerType = HeaderManager;
  template <int64_t num_bytes_t>
  using MemoryPoolType = MemoryPoolEqual<num_bytes_t>;
  template <int64_t num_bytes_t>
  using MemoryPoolPtrType = std::unique_ptr<MemoryPoolType<num_bytes_t>>;

  enum struct ePoolSize {
    Small = 1,
    Medium = 2,
    Large = 3,
    Malloc = 4
  };

  Pool();

  std::string name() override { return "MemoryPool"; }

  void* alloc(size_t const& num_bytes, size_t oversize = 0);
  void dealloc(void* const buf);
  ePoolSize getPoolType(size_t const& num_bytes, size_t const& oversize);
  SizeType remainingSize(void* const buf);
  bool active() const;
  bool active_env() const;

  void initWorkerPools(WorkerCountType const& num_workers);
  void finalize() override;

private:
  /*
   * Attempt allocation via pooled and fall back to default allocation if it
   * fails
   */
  void* tryPooledAlloc(size_t const& num_bytes, size_t const& oversize);
  bool tryPooledDealloc(void* const buf);

  /*
   * Allocate memory from a specific local memory pool, indicated by `pool'
   */
  void* pooledAlloc(
    size_t const& num_bytes, size_t const& oversize, ePoolSize const pool_type
  );
  void poolDealloc(void* const buf, ePoolSize const pool_type);

  /*
   * Allocate from the default system allocator (std::malloc)
   */
  void* defaultAlloc(size_t const& num_bytes, size_t const& oversize);
  void defaultDealloc(void* const ptr);

private:
  using MemPoolSType = MemoryPoolPtrType<memory_size_small>;
  using MemPoolMType = MemoryPoolPtrType<memory_size_medium>;

  static MemPoolSType initSPool();
  static MemPoolMType initMPool();

private:
  MemPoolSType small_msg = nullptr;
  MemPoolMType medium_msg = nullptr;

  std::vector<MemPoolSType> s_msg_worker_;
  std::vector<MemPoolMType> m_msg_worker_;
};

}} //end namespace vt::pool

namespace vt {

extern pool::Pool* thePool();

} //end namespace vt

#endif /*INCLUDED_POOL_POOL_H*/
