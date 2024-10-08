/*
//@HEADER
// *****************************************************************************
//
//                                   holder.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_RDMAHANDLE_HOLDER_H
#define INCLUDED_VT_RDMAHANDLE_HOLDER_H

#include "vt/config.h"
#include "vt/rdmahandle/common.h"
#include "vt/rdmahandle/request_holder.h"
#include "vt/rdmahandle/lock_mpi.h"

#include <unordered_map>
#include <vector>
#include <memory>

namespace vt { namespace rdma {

struct Manager;

template <typename T, HandleEnum E>
struct Holder {
  Holder() = default;

  bool ready() const { return ready_; }

  friend struct Manager;

private:
  template <typename ProxyT>
  void addHandle(
    HandleKey key, ElemType lin, Handle<T,E> han, std::size_t size,
    bool uniform_size
  );
  void allocateDataWindow(std::size_t const in_len = 0);

public:
  std::shared_ptr<LockMPI> lock(Lock l, vt::NodeType node);

public:
  void deallocate();

  template <typename Callable>
  void access(Lock l, Callable fn, std::size_t offset);

  std::size_t getCount(vt::NodeType node, Lock l = Lock::Shared);

  RequestHolder rget(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
  );
  void get(vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset);

  RequestHolder rput(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
  );
  void put(vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset);

  RequestHolder raccum(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset,
    MPI_Op op
  );
  void accum(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset,
    MPI_Op op
  );

  T fetchOp(vt::NodeType node, Lock l, T ptr, int offset, MPI_Op op);

  void fence(int assert = 0);
  void sync();
  void flush(vt::NodeType node);
  void flushLocal(vt::NodeType node);
  void flushAll();

  bool isUniform() const { return uniform_size_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | key_
      | data_base_
      | control_base_
      | count_
      | ready_
      | mpi2_
      | uniform_size_
      | handle_
      | data_window_
      | control_window_;
  }

private:
  HandleKey key_;
  MPI_Win data_window_;
  MPI_Win control_window_;
  T* data_base_ = nullptr;
  uint64_t* control_base_ = nullptr;
  std::size_t count_ = 0;
  bool ready_ = false;
  bool mpi2_ = false;
  bool uniform_size_ = false;
  Handle<T,E> handle_;
};

}} /* end namespace vt::rdma */

#include "vt/rdmahandle/holder.impl.h"

#endif /*INCLUDED_VT_RDMAHANDLE_HOLDER_H*/
