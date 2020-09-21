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

/**
 * \struct Pool
 *
 * \brief A core VT component that manages efficient pools of memory for quick
 * allocation/deallocation.
 *
 * Highly efficient memory pool that is not thread-safe. Utilizes fixed-size
 * buckets with free-list to quickly allocate and de-allocate.
 */
struct Pool : runtime::component::Component<Pool> {
  using SizeType = size_t;
  using HeaderType = Header;
  using HeaderManagerType = HeaderManager;
  template <int64_t num_bytes_t>
  using MemoryPoolType = MemoryPoolEqual<num_bytes_t>;
  template <int64_t num_bytes_t>
  using MemoryPoolPtrType = std::unique_ptr<MemoryPoolType<num_bytes_t>>;

  /**
   * \brief Different pool sizes: small, medium, large, and the backup malloc
   */
  enum struct ePoolSize {
    Small = 1,                  /**< Small bucket */
    Medium = 2,                 /**< Medium bucket */
    Large = 3,                  /**< Large bucket */
    Malloc = 4                  /**< Backup malloc allocation */
  };

  /**
   * \internal \brief System construction of the pool component
   */
  Pool();

  std::string name() override { return "MemoryPool"; }

  /**
   * \brief Allocate some number of bytes plus extra size at the end
   *
   * \param[in] num_bytes main payload
   * \param[in] oversize extra bytes
   *
   * \return pointer to new allocation
   */
  void* alloc(size_t const& num_bytes, size_t oversize = 0);

  /**
   * \brief De-allocate a pool-allocated buffer
   *
   * \param[in] buf the buffer to deallocate
   */
  void dealloc(void* const buf);

  /**
   * \internal \brief Decided which pool bucket to target based on size
   *
   * \param[in] num_bytes main payload
   * \param[in] oversize extra bytes
   *
   * \return enum \c ePoolSize of which pool to target
   */
  ePoolSize getPoolType(size_t const& num_bytes, size_t const& oversize);

  /**
   * \internal \brief Get remaining bytes for a pool allocation
   *
   * When using the memory pool, often extra bytes are at the end of the
   * allocation because the user did not request the whole block assigned. Some
   * components use this extra memory to pack in extra meta-data (or send
   * serialized data) when sending a message.
   *
   * \param[in] buf the buffer allocated from the pool
   *
   * \return number of extra bytes
   */
  SizeType remainingSize(void* const buf);

  /**
   * \brief Whether the pool is enabled at compile-time
   *
   * \return whether its enabled
   */
  bool active() const;

  /**
   * \brief Whether the pool is enabled at compile-time and used as the default
   * allocator for messages
   *
   * \return whether its enabled
   */
  bool active_env() const;

  /**
   * \brief Initialize worker-specific pools due to the lack of thread-safety of
   * the memory allocator. This will create distinct memory pool instances for
   * each worker thread to access
   *
   * \param[in] num_workers number of workers on this node
   */
  void initWorkerPools(WorkerCountType const& num_workers);

  /**
   * \brief Cleanup/free the memory pools
   */
  void finalize() override;

  template <typename Serializer>
  void serialize(Serializer& s) {
    // s | small_msg // missing support for void*
    //   | medium_msg
    //   | s_msg_worker_
    //   | m_msg_worker_;
  }

private:
  /**
   * \internal \brief Attempt allocation via pooled allocator and fall back to
   * standard allocation if it fails
   *
   * \param[in] num_bytes main payload size
   * \param[in] oversize extra size requested
   *
   * \return a pointer to memory if succeeds
   */
  void* tryPooledAlloc(size_t const& num_bytes, size_t const& oversize);

  /**
   * \internal \brief Attempt to de-allocate a buffer
   *
   * \param[in] buf buffer to deallocate
   *
   * \return whether it succeeded or wasn't allocated by the pool
   */
  bool tryPooledDealloc(void* const buf);

  /**
   * \internal \brief Allocate memory from a specific pool
   *
   * \param[in] num_bytes main payload size
   * \param[in] oversize extra size requested
   * \param[in] pool_type the pool to target of sufficient size
   *
   * \return the buffer allocated
   */
  void* pooledAlloc(
    size_t const& num_bytes, size_t const& oversize, ePoolSize const pool_type
  );

  /**
   * \internal \brief De-allocate memory from pool
   *
   * \param[in] buf the buffer
   * \param[in] pool_type which pool to target
   */
  void poolDealloc(void* const buf, ePoolSize const pool_type);

  /**
   * \internal \brief Allocate from standard allocator
   *
   * \param[in] num_bytes main payload size
   * \param[in] oversize extra size requested
   *
   * \return the allocated buffer
   */
  void* defaultAlloc(size_t const& num_bytes, size_t const& oversize);

  /**
   * \internal \brief De-allocate from standard allocator
   *
   * \param[in] ptr buffer to deallocate
   */
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
