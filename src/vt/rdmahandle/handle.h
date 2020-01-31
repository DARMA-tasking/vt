/*
//@HEADER
// *****************************************************************************
//
//                                   handle.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_H

#include "vt/config.h"
#include "vt/rdmahandle/handle_key.h"
#include "vt/rdmahandle/request_holder.h"
#include "vt/rdmahandle/lock_mpi.h"

#include <functional>
#include <vector>
#include <memory>

namespace vt { namespace rdma {

enum struct HandleEnum {
  StaticSize = 1,
  ConcurrentList = 2
};

struct BaseHandle { };

struct Manager;

template <typename T, HandleEnum E, typename IndexT = vt::NodeType>
struct Handle : BaseHandle {
  using DataT = T;
  using RequestType = RequestHolder;
  using ActionDataType = std::function<void(T*)>;

  template <typename U>
  using isNodeType =
    typename std::enable_if<std::is_same<IndexT,vt::NodeType>::value, U>::type;

  template <typename U>
  using isIndexType =
    typename std::enable_if<not std::is_same<IndexT,vt::NodeType>::value, U>::type;

  static constexpr HandleEnum handle_type = E;

  Handle() = default;
  Handle(Handle const&) = default;
  Handle(Handle&&) = default;
  Handle& operator=(Handle const&) = default;

  friend struct Manager;

private:
  Handle(
    HandleKey in_key, std::size_t in_size, std::size_t in_hoff = 0,
    std::shared_ptr<LockMPI> in_lock = nullptr
  ) : key_(in_key),
      size_(in_size),
      hoff_(in_hoff),
      lock_(in_lock)
  { }

public:
  Handle<T,E> sub(std::size_t in_offset, std::size_t in_size = 0) {
    if (in_size == 0) {
      in_size = size_ - in_offset;
    }
    return Handle<T,E>(key_, in_size, hoff_ + in_offset, lock_);
  }

public:
  bool isInit() const { return key_.valid(); }
  bool ready() const;
  bool hasAction() const { return actions_.size() > 0; }
  bool clearActions() const { return actions_.clear(); }
  void addAction(ActionDataType in_action) { actions_.push_back(in_action); }
  void setBuffer(T* in_buffer) { user_buffer_ = in_buffer; }
  T* getBuffer() const { return user_buffer_; }

public:
  template <typename U = IndexT>
  void readExclusive(std::function<void(T const*)> fn, isNodeType<U>* = nullptr);
  template <typename U = IndexT>
  void readShared(std::function<void(T const*)> fn, isNodeType<U>* = nullptr);
  template <typename U = IndexT>
  void modifyExclusive(std::function<void(T*)> fn, isNodeType<U>* = nullptr);
  template <typename U = IndexT>
  void modifyShared(std::function<void(T*)> fn, isNodeType<U>* = nullptr);

public:
  void lock(Lock l, vt::NodeType node);
  void unlock();

public:
  template <typename U = IndexT>
  void get(
    U, std::size_t len, int offset, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  void get(
    U, T* ptr, std::size_t len, int offset, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  void put(
    U, T* ptr, std::size_t len, int offset, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  void accum(
    U, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  RequestType rget(
    U, T* ptr, std::size_t len, int offset, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  RequestType rget(
    U, std::size_t len, int offset, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  RequestType rput(
    U, T* ptr, std::size_t len, int offset, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );
  template <typename U = IndexT>
  RequestType raccum(
    U, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l = Lock::None,
    isNodeType<U>* = nullptr
  );

  template <typename U = IndexT>
  std::size_t getSize(U, isNodeType<U>* = nullptr);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | key_;
    s | size_;
    s | hoff_;
  }

protected:
  HandleKey key_ = {};
  std::size_t size_  = 0;
  std::vector<ActionDataType> actions_ = {};
  T* user_buffer_ = nullptr;
  std::size_t hoff_ = 0;
  std::shared_ptr<LockMPI> lock_ = nullptr;
};

}} /* end namespace vt::rdma */

namespace vt {

template <typename T>
using HandleRDMA = rdma::Handle<T, rdma::HandleEnum::StaticSize, vt::NodeType>;

template <typename T>
using HandleListRDMA = rdma::Handle<T, rdma::HandleEnum::ConcurrentList, vt::NodeType>;

} /* end namespace vt */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_H*/
