/*
//@HEADER
// *****************************************************************************
//
//                                handle.index.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_INDEX_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_INDEX_H

#include "vt/config.h"
#include "vt/rdmahandle/base_handle.h"
#include "vt/rdmahandle/handle.fwd.h"
#include "vt/rdmahandle/handle_key.h"
#include "vt/rdmahandle/request_holder.h"
#include "vt/rdmahandle/lock_mpi.h"

#include <functional>
#include <vector>
#include <memory>

namespace vt { namespace rdma {

/** \file */

/**
 * \struct Handle handle.index.h vt/rdmahandle/handle.index.h
 *
 * \brief Handle specialization for index-level RDMA access (overdecomposed)
 */
template <typename T, HandleEnum E, typename IndexT>
struct Handle<
  T, E, IndexT,
  typename std::enable_if_t<
    not std::is_same<IndexT,vt::NodeType>::value
  >
> : BaseTypedHandle<T, E, IndexT>
{
  using RequestType = typename BaseTypedHandle<T, E, IndexT>::RequestType;

  Handle() = default;
  Handle(Handle const&) = default;
  Handle(Handle&&) = default;
  Handle& operator=(Handle const&) = default;
  Handle& operator=(Handle&&) = default;

  friend struct Manager;
  friend struct SubHandle<T, E, IndexT>;

public:
  struct IndexTagType { };

  /**
   * \brief Private constructor used the system factory method to construct a
   * new index-level handle
   *
   * \param[in] in_proxy the proxy for managing the indexed handle
   * \param[in] in_index the index for this handle
   * \param[in] in_size size of local handle
   * \param[in] in_hoff local offset for handle
   */
  Handle(
    IndexTagType,
    ObjGroupProxyType in_proxy, IndexT const& in_index, std::size_t in_size,
    std::size_t in_hoff = 0
  ) : BaseTypedHandle<T, E, IndexT>(
        in_size,
        in_hoff,
        std::shared_ptr<LockMPI>{}
      ),
      index_(in_index),
      proxy_(in_proxy)
  { }

public:
  /**
   * \brief Check if the handle is initialized
   *
   * \return whether the handle is initialized
   */
  bool isInit() const { return proxy_ != no_obj_group; }

  /**
   * \brief Get the index for this handle
   *
   * \return the index
   */
  IndexT const& getIndex() const { return index_; }

  /**
   * \brief Check if the handle is ready to be used; implies that all MPI
   * windows are constructed across the machine
   *
   * \return whether its ready
   */
  bool ready() const;

public:

  /**
   * \brief Read the local data for the handle with an exclusive lock
   *
   * \param[in] idx this index to read
   * \param[in] fn lambda to read the data
   */
  void readExclusive(IndexT const& idx, std::function<void(T const*)> fn);

  /**
   * \brief Read the local data for the handle with an shared lock
   *
   * \param[in] idx this index to read
   * \param[in] fn lambda to read the data
   */
  void readShared(IndexT const& idx, std::function<void(T const*)> fn);

  /**
   * \brief Modify the local data for the handle with an exclusive lock
   *
   * \param[in] idx this index to read
   * \param[in] fn lambda to modify the data
   */
  void modifyExclusive(IndexT const& idx, std::function<void(T*)> fn);

  /**
   * \brief Modify the local data for the handle with an shared lock
   *
   * \param[in] idx this index to read
   * \param[in] fn lambda to modify the data
   */
  void modifyShared(IndexT const& idx, std::function<void(T*)> fn);

public:

  /**
   * \brief Get data from the underlying MPI window on a remote index, calls
   * down to \c MPI_Get or \c MPI_Rget
   *
   * This variant of \c get fetches into the buffer that is associated with the
   * the handle. See \c setBuffer and \c getBuffer
   *
   * \param[in] idx the handle index to target
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   */
  void get(
    IndexT const& idx, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Get data from the underlying MPI window on a remote process, calls
   * down to \c MPI_Get or \c MPI_Rget
   *
   * \param[in] idx the handle index to target
   * \param[out] ptr the pointer to write the fetched bytes to
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   */
  void get(
    IndexT const& idx, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Put data using the underlying MPI window on a remote process, calls
   * down to \c MPI_Put or \c MPI_Rput
   *
   * \param[in] idx the handle index to target
   * \param[in] ptr the pointer of data to put
   * \param[in] len the length to put
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the put
   */
  void put(
    IndexT const& idx, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Accumulate data using the underlying MPI window on a remote process,
   * calls down to \c MPI_Accumulate or \c MPI_Raccumulate
   *
   * \param[in] idx the handle index to target
   * \param[in] ptr the pointer of data to accumulate
   * \param[in] len the length to accumulate
   * \param[in] offset the offset
   * \param[in] op the operation \c MPI_Op to apply for accumulate
   * \param[in] l the lock to apply for the accumulate
   */
  void accum(
    IndexT const& idx, T* ptr, std::size_t len, int offset, MPI_Op op,
    Lock l = Lock::None
  );

  /**
   * \brief Get data asynchronously from the underlying MPI window on a remote
   * process, calls down to \c MPI_Rget when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Get to the
   * \c RequestHolder that is dispatched when waited on. This variant of \c get
   * fetches into the buffer that is associated with the the handle. See \c
   * setBuffer and \c getBuffer
   *
   * \param[in] idx the handle index to target
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   *
   * \return the request holder to wait on
   */
  RequestType rget(
    IndexT const& idx, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Get data asynchronously from the underlying MPI window on a remote
   * process, calls down to \c MPI_Rget when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Get to the \c
   * RequestHolder that is dispatched when waited on.
   *
   * \param[in] idx the handle index to target
   * \param[out] ptr the pointer to write the fetched bytes to
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   *
   * \return the request holder to wait on
   */
  RequestType rget(
    IndexT const& idx, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Put data asynchronously from the underlying MPI window on a remote
   * process, calls down to \c MPI_Rput when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Put to the
   * \c RequestHolder that is dispatched when waited on.
   *
   * \param[in] idx the handle index to target
   * \param[in] ptr the pointer of data to put
   * \param[in] len the length to put
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the put
   */
  RequestType rput(
    IndexT const& idx, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Accumulate data asynchronously using the underlying MPI window on a
   * remote process, calls down to \c MPI_Raccumulate when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Accumulate to the
   * \c RequestHolder that is dispatched when waited on.
   *
   * \param[in] idx the handle index to target
   * \param[in] ptr the pointer of data to accumulate
   * \param[in] len the length to accumulate
   * \param[in] offset the offset
   * \param[in] op the operation \c MPI_Op to apply for accumulate
   * \param[in] l the lock to apply for the accumulate
   */
  RequestType raccum(
    IndexT const& idx, T* ptr, std::size_t len, int offset, MPI_Op op,
    Lock l = Lock::None
  );

  /**
   * \brief Perform one-sided read-modify-write using the underlying MPI window
   * on a remote process, calls down to \c MPI_Fetch_and_op.
   *
   * \param[in] idx the handle index to target
   * \param[in] val the value to fetch-op
   * \param[in] offset the offset
   * \param[in] op the operation \c MPI_Op to apply for fetch-op
   * \param[in] l the lock to apply for the fetch-op
   *
   * \return the value fetched
   */
  T fetchOp(
    IndexT const& idx, T val, int offset, MPI_Op op, Lock l = Lock::None
  );

  /**
   * \brief Get the size of the data window for a certain index. If the size is
   * non-uniform, it will remotely fetch the size from that node.
   *
   * \param[in] idx the index to request the size
   *
   * \return the length of the handle's data
   */
  std::size_t getSize(IndexT const& idx);

  /**
   * \brief Lock the handle to apply multiple operations
   *
   * \param[in] l lock to apply
   * \param[in] idx which idx to lock
   */
  void lock(Lock l, IndexT const& node);

  /**
   * \brief Unlock the handle
   */
  void unlock();

  /**
   * \brief Serializer for the handle
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | index_;
    s | proxy_;
    BaseTypedHandle<T, E, IndexT>::serialize(s);
  }

protected:
  IndexT index_            = {};           /**< The index for the handle */
  ObjGroupProxyType proxy_ = no_obj_group; /**< The managing objgroup proxy */
  //HandleKey key_           = {};           /**< The key for the collection  */
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_INDEX_H*/
