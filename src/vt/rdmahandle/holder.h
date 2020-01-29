/*
//@HEADER
// *****************************************************************************
//
//                                   holder.h
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
  void addHandle(HandleKey key, ElemType lin, Handle<T,E> han, std::size_t in_size) {
    handles_[lin] = han;
    key_ = key;
    size_ += in_size;
  }

  void allocateDataWindow(std::size_t const in_len = 0) {
    std::size_t len = in_len == 0 ? size_ : in_len;
    fmt::print("allocate: len={}\n", len);
    MPI_Alloc_mem(len * sizeof(T), MPI_INFO_NULL, &data_base_);
    MPI_Win_create(data_base_, len * sizeof(T), sizeof(T), MPI_INFO_NULL, MPI_COMM_WORLD, &data_window_);
    ready_ = true;
  }

public:
  std::shared_ptr<LockMPI> lock(Lock l, vt::NodeType node) {
    return std::make_shared<LockMPI>(l, node, data_window_);
  }

public:
  template <typename Callable>
  void access(Lock l, Callable fn) {
    auto this_node = theContext()->getNode();

    LockMPI _scope_lock(l, this_node, data_window_);
    fn(data_base_);
  }

  RequestHolder rget(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
  ) {
    auto mpi_type = TypeMPI<T>::getType();
    RequestHolder r;
    {
      LockMPI _scope_lock(l, node, data_window_);
      MPI_Rget(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_, r.add());
    }
    return r;
  }

  void get(vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset) {
    rget(node, l, ptr, len, offset);
  }

  RequestHolder rput(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
  ) {
    auto mpi_type = TypeMPI<T>::getType();
    RequestHolder r;
    {
      LockMPI _scope_lock(l, node, data_window_);
      MPI_Rput(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_, r.add());
    }
    return r;
  }

  void put(vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset) {
    rput(node, l, ptr, len, offset);
  }

  RequestHolder raccum(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset,
    MPI_Op op
  ) {
    auto mpi_type = TypeMPI<T>::getType();
    RequestHolder r;
    {
      LockMPI _scope_lock(l, node, data_window_);
      MPI_Raccumulate(
        ptr, len, mpi_type, node, offset, len, mpi_type, op, data_window_, r.add()
      );
    }
    return r;
  }

  void accum(
    vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset,
    MPI_Op op
  ) {
    raccum(node, l, ptr, len, offset, op);
  }


private:
  HandleKey key_;
  MPI_Win data_window_;
  MPI_Win idx_window_;
  MPI_Win control_window_;
  T* data_base_ = nullptr;
  T* idx_base_ = nullptr;
  T* control_base_ = nullptr;
  std::size_t size_ = 0;
  bool ready_ = false;

private:
  std::unordered_map<ElemType, Handle<T,E>> handles_;
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HOLDER_H*/
