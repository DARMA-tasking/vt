/*
//@HEADER
// *****************************************************************************
//
//                                handle.node.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_NODE_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_NODE_H

#include "vt/config.h"
#include "vt/rdmahandle/common.h"
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
 * \struct Handle handle.node.h vt/rdmahandle/handle.node.h
 *
 * \brief Handle specialization for node-level RDMA access (non-overdecomposed)
 */
template <typename T, HandleEnum E, typename IndexT>
struct Handle<
  T, E, IndexT,
  typename std::enable_if_t<
    std::is_same<IndexT,vt::NodeType>::value
  >
> : BaseTypedHandle<T, E, vt::NodeType>
{
  using RequestType = typename BaseTypedHandle<T, E, vt::NodeType>::RequestType;

  friend struct Manager;
  friend struct SubHandle<T, E, IndexT>;

  Handle() = default;
  Handle(Handle const&) = default;
  Handle(Handle&&) = default;
  Handle& operator=(Handle const&) = default;
  Handle& operator=(Handle&&) = default;

  struct NodeTagType { };

private:
  /**
   * \brief Private constructor used the system factor method to construct a new
   * node-level handle
   *
   * \param[in] NodeTagType tag for getting this constructor
   * \param[in] in_key the key for identifying the handle
   * \param[in] in_size size of local handle
   * \param[in] in_hoff local offset for handle
   * \param[in] in_lock handle lock
   */
  Handle(
    NodeTagType,
    HandleKey in_key, std::size_t in_size, std::size_t in_hoff = 0,
    std::shared_ptr<LockMPI> in_lock = nullptr
  ) : BaseTypedHandle<T, E, vt::NodeType>(
        in_size,
        in_hoff,
        in_lock
      ),
      key_(in_key)
  { }

public:
  /**
   * \brief Check if the handle is initialized
   *
   * \return whether the handle is initialized
   */
  bool isInit() const { return key_.valid(); }

  /**
   * \brief Get the node this handle lives on
   *
   * \return the node
   */
  vt::NodeType getNode() const { return vt::theContext()->getNode(); }

  /**
   * \brief Check if the handle is ready to be used; implies that all MPI
   * windows are constructed across the machine
   *
   * \return whether its ready
   */
  bool ready() const;

  /**
   * \brief Read the local data for the handle with an exclusive lock
   *
   * \param[in] fn lambda to read the data
   */
  void readExclusive(std::function<void(T const*)> fn);

  /**
   * \brief Read the local data for the handle with an shared lock
   *
   * \param[in] fn lambda to read the data
   */
  void readShared(std::function<void(T const*)> fn);

  /**
   * \brief Modify the local data for the handle with an exclusive lock
   *
   * \param[in] fn lambda to modify the data
   */
  void modifyExclusive(std::function<void(T*)> fn);

  /**
   * \brief Modify the local data for the handle with an shared lock
   *
   * \param[in] fn lambda to modify the data
   */
  void modifyShared(std::function<void(T*)> fn);

  /**
   * \brief General access function for reading/modifying local data
   *
   * \param[in] l lock to apply
   * \param[in] fn lambda to access data
   * \param[in] size number of elements to read
   */
  void access(Lock l, std::function<void(T*)> fn, std::size_t size);

  /**
   * \brief Lock the handle to apply multiple operations
   *
   * \param[in] l lock to apply
   * \param[in] node which node to lock
   */
  void lock(Lock l, vt::NodeType node);

  /**
   * \brief Unlock the handle
   */
  void unlock();

  /**
   * \brief Perform fence synchronization on the underlying data window
   *
   * Applies an \c MPI_Win_fence to the underlying data window. One may specify
   * the strength of the fence as the \c assert argument
   *
   * \c MPI_MODE_NOSTORE --- the local window was not updated by local stores
   * (or local get or receive calls) since last synchronization.
   *
   * \c MPI_MODE_NOPUT --- the local window will not be updated by put or
   * accumulate calls after the fence call, until the ensuing (fence)
   * synchronization.
   *
   * \c MPI_MODE_NOPRECEDE --- the fence does not complete any sequence of
   * locally issued RMA calls. If this assertion is given by any process in the
   * window group, then it must be given by all processes in the group.
   *
   * \c MPI_MODE_NOSUCCEED --- the fence does not start any sequence of locally
   * issued RMA calls. If the assertion is given by any process in the window
   * group, then it must be given by all processes in the group.
   *
   * \param[in] assert program assertion (may be used for optimization)
   */
  void fence(int assert = 0);

  /**
   * \brief Perform a window sync on the data window \c MPI_Win_sync
   */
  void sync();

  /**
   * \brief Perform a window flush on the data window \c MPI_Win_flush for a
   * certain node
   */
  void flush(vt::NodeType node);

  /**
   * \brief Perform a local window flush on the data window \c
   * MPI_Win_flush_local for a certain node
   */
  void flushLocal(vt::NodeType node);

  /**
   * \brief Perform a window flush on the data window \c MPI_Win_flush_all for
   * all nodes
   */
  void flushAll();

public:
  /**
   * \brief Get data from the underlying MPI window on a remote process, calls
   * down to \c MPI_Get or \c MPI_Rget
   *
   * This variant of \c get fetches into the buffer that is associated with the
   * the handle. See \c setBuffer and \c getBuffer
   *
   * \param[in] node the process/node to target
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   */
  void get(
    vt::NodeType node, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Get data from the underlying MPI window on a remote process, calls
   * down to \c MPI_Get or \c MPI_Rget
   *
   * \param[in] node the process/node to target
   * \param[out] ptr the pointer to write the fetched bytes to
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   */
  void get(
    vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Put data using the underlying MPI window on a remote process, calls
   * down to \c MPI_Put or \c MPI_Rput
   *
   * \param[in] node the process/node to target
   * \param[in] ptr the pointer of data to put
   * \param[in] len the length to put
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the put
   */
  void put(
    vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Accumulate data using the underlying MPI window on a remote process,
   * calls down to \c MPI_Accumulate or \c MPI_Raccumulate
   *
   * \param[in] node the process/node to target
   * \param[in] ptr the pointer of data to accumulate
   * \param[in] len the length to accumulate
   * \param[in] offset the offset
   * \param[in] op the operation \c MPI_Op to apply for accumulate
   * \param[in] l the lock to apply for the accumulate
   */
  void accum(
    vt::NodeType node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l = Lock::None
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
   * \param[in] node the process/node to target
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   *
   * \return the request holder to wait on
   */
  RequestType rget(
    vt::NodeType no, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Get data asynchronously from the underlying MPI window on a remote
   * process, calls down to \c MPI_Rget when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Get to the \c
   * RequestHolder that is dispatched when waited on.
   *
   * \param[in] node the process/node to target
   * \param[out] ptr the pointer to write the fetched bytes to
   * \param[in] len the length to get
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the get
   *
   * \return the request holder to wait on
   */
  RequestType rget(
    vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Put data asynchronously from the underlying MPI window on a remote
   * process, calls down to \c MPI_Rput when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Put to the
   * \c RequestHolder that is dispatched when waited on.
   *
   * \param[in] node the process/node to target
   * \param[in] ptr the pointer of data to put
   * \param[in] len the length to put
   * \param[in] offset the offset
   * \param[in] l the lock to apply for the put
   */
  RequestType rput(
    vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l = Lock::None
  );

  /**
   * \brief Accumulate data asynchronously using the underlying MPI window on a
   * remote process, calls down to \c MPI_Raccumulate when MPI 3 is enabled.
   *
   * If MPI 3 is not enabled, it attaches a regular \c MPI_Accumulate to the
   * \c RequestHolder that is dispatched when waited on.
   *
   * \param[in] node the process/node to target
   * \param[in] ptr the pointer of data to accumulate
   * \param[in] len the length to accumulate
   * \param[in] offset the offset
   * \param[in] op the operation \c MPI_Op to apply for accumulate
   * \param[in] l the lock to apply for the accumulate
   */
  RequestType raccum(
    vt::NodeType node, T* ptr, std::size_t len, int offset, MPI_Op op,
    Lock l = Lock::None
  );

  /**
   * \brief Perform one-sided read-modify-write using the underlying MPI window on a
   * remote process, calls down to \c MPI_Fetch_and_op.
   *
   * \param[in] node the process/node to target
   * \param[in] val the value to fetch-op
   * \param[in] offset the offset
   * \param[in] op the operation \c MPI_Op to apply for fetch-op
   * \param[in] l the lock to apply for the fetch-op
   *
   * \return the value fetched
   */
  T fetchOp(
    vt::NodeType node, T val, int offset, MPI_Op op, Lock l = Lock::None
  );

  /**
   * \brief Get the size of the data window on a certain node. If the size is
   * non-uniform, it will remotely fetch the size from that node.
   *
   * \param[in] node the node to request the size
   *
   * \return the length of the handle's data
   */
  std::size_t getSize(vt::NodeType node);

  /**
   * \brief Serializer for the handle
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | key_;
    BaseTypedHandle<T, E, vt::NodeType>::serialize(s);
  }

protected:
  HandleKey key_ = {};          /**< The key for identifying the handle */
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_NODE_H*/
