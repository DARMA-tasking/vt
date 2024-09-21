/*
//@HEADER
// *****************************************************************************
//
//                                holder.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HOLDER_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_HOLDER_IMPL_H

#include "vt/config.h"
#include "vt/runtime/mpi_access.h"

namespace vt { namespace rdma {


template <typename T, HandleEnum E>
template <typename ProxyT>
void Holder<T,E>::addHandle(
  HandleKey key, [[maybe_unused]] ElemType lin,
  Handle<T,E> han, std::size_t in_count, bool uniform_size
) {
  handle_ = han;
  key_ = key;
  count_ += in_count;
  uniform_size_ = uniform_size;
}

template <typename T, HandleEnum E>
void Holder<T,E>::allocateDataWindow(std::size_t const in_len) {
  std::size_t len = in_len == 0 ? count_ : in_len;
  vt_debug_print(
    terse, rdma,
    "allocate: len={}, in_len={}, count_={}\n", len, in_len, count_
  );
  // Allocate data window
  MPI_Comm comm = theContext()->getComm();
  MPI_Alloc_mem(len * sizeof(T), MPI_INFO_NULL, &data_base_);
  MPI_Win_create(
    data_base_, len * sizeof(T), sizeof(T), MPI_INFO_NULL, comm,
    &data_window_
  );
  if (not uniform_size_) {
    // Allocate control window
    MPI_Alloc_mem(sizeof(uint64_t), MPI_INFO_NULL, &control_base_);
    MPI_Win_create(
      control_base_, sizeof(uint64_t), sizeof(uint64_t), MPI_INFO_NULL, comm,
      &control_window_
    );
    {
      auto this_node = theContext()->getNode();
      LockMPI _scope_lock(Lock::Exclusive, this_node, control_window_, false);
      auto mpi_type = TypeMPI<uint64_t>::getType();
      MPI_Put(&count_, 1, mpi_type, this_node, 0, 1, mpi_type, control_window_);
      vt_debug_print(
        verbose, rdma,
        "setting allocate size: size={}, window={}\n",
        count_, print_ptr(&control_window_)
      );
    }
  }
  ready_ = true;
}

template <typename T, HandleEnum E>
std::size_t Holder<T,E>::getCount(vt::NodeType node, Lock l) {
  uint64_t result = 0;
  auto mpi_type = TypeMPI<uint64_t>::getType();
  {
    LockMPI _scope_lock(l, node, control_window_);
    VT_ALLOW_MPI_CALLS;
    MPI_Get(&result, 1, mpi_type, node, 0, 1, mpi_type, control_window_);
  }
  vt_debug_print(
    verbose, rdma,
    "getCount: node={}, result={}, window={}\n",
    node, result, print_ptr(&control_window_)
  );
  return static_cast<std::size_t>(result);
}

template <typename T, HandleEnum E>
void Holder<T,E>::deallocate() {
  VT_ALLOW_MPI_CALLS;
  if (E == HandleEnum::StaticSize and ready_) {
    MPI_Win_free(&data_window_);
    MPI_Free_mem(data_base_);
    if (not uniform_size_) {
      MPI_Win_free(&control_window_);
      MPI_Free_mem(control_base_);
    }
  }
}

template <typename T, HandleEnum E>
std::shared_ptr<LockMPI> Holder<T,E>::lock(Lock l, vt::NodeType node) {
  return std::make_shared<LockMPI>(l, node, data_window_);
}

template <typename T, HandleEnum E>
template <typename Callable>
void Holder<T,E>::access(Lock l, Callable fn, std::size_t offset) {
  auto this_node = theContext()->getNode();

  LockMPI _scope_lock(l, this_node, data_window_);
  fn(data_base_ + offset, count_ - offset);
}

template <typename T, HandleEnum E>
RequestHolder Holder<T,E>::rget(
  vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
) {
  auto mpi_type = TypeMPI<T>::getType();
  auto mpi_type_str = TypeMPI<T>::getTypeStr();
  RequestHolder r;
  if (mpi2_) {
    r.add([=]{
      LockMPI _scope_lock(l, node, data_window_);
      vt_debug_print(
        verbose, rdma,
        "MPI_Get({}, {}, {}, {}, {}, {}, {}, window);\n",
        print_ptr(ptr), len, mpi_type_str, node, offset, len, mpi_type_str
      );
      VT_ALLOW_MPI_CALLS;
      MPI_Get(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_);
    });
  } else {
    LockMPI _scope_lock(l, node, data_window_);
    vt_debug_print(
      verbose, rdma,
      "MPI_Rget({}, {}, {}, {}, {}, {}, {}, window);\n",
      print_ptr(ptr), len, mpi_type_str, node, offset, len, mpi_type_str
    );
    VT_ALLOW_MPI_CALLS;
    MPI_Rget(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_, r.add());
  }
  return r;
}

template <typename T, HandleEnum E>
void Holder<T,E>::get(
  vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
) {
  rget(node, l, ptr, len, offset);
}

template <typename T, HandleEnum E>
RequestHolder Holder<T,E>::rput(
  vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
) {
  auto mpi_type = TypeMPI<T>::getType();
  auto mpi_type_str = TypeMPI<T>::getTypeStr();
  RequestHolder r;
  if (mpi2_) {
    r.add([=]{
      LockMPI _scope_lock(l, node, data_window_);
      vt_debug_print(
        verbose, rdma,
        "MPI_Put({}, {}, {}, {}, {}, {}, {}, window);\n",
        print_ptr(ptr), len, mpi_type_str, node, offset, len, mpi_type_str
      );
      VT_ALLOW_MPI_CALLS;
      MPI_Put(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_);
    });
  } else {
    LockMPI _scope_lock(l, node, data_window_);
    vt_debug_print(
      verbose, rdma,
      "MPI_Rput({}, {}, {}, {}, {}, {}, {}, window);\n",
      print_ptr(ptr), len, mpi_type_str, node, offset, len, mpi_type_str
    );
    VT_ALLOW_MPI_CALLS;
    MPI_Rput(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_, r.add());
  }
  return r;
}

template <typename T, HandleEnum E>
void Holder<T,E>::put(
  vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset
) {
  rput(node, l, ptr, len, offset);
}

template <typename T, HandleEnum E>
T Holder<T,E>::fetchOp(vt::NodeType node, Lock l, T in, int offset, MPI_Op op) {
  auto mpi_type = TypeMPI<T>::getType();
  auto mpi_type_str = TypeMPI<T>::getTypeStr();
  T out;
  {
    LockMPI _scope_lock(l, node, data_window_);
    vt_debug_print(
      verbose, rdma,
      "MPI_Fetch_and_op({}, {}, {}, {}, {}, window);\n",
      in, print_ptr(&out), mpi_type_str, node, offset
    );
    VT_ALLOW_MPI_CALLS;
    MPI_Fetch_and_op(&in, &out, mpi_type, node, offset, op, data_window_);
  }
  return out;
}

template <typename T, HandleEnum E>
RequestHolder Holder<T,E>::raccum(
  vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset,
  MPI_Op op
) {
  auto mpi_type = TypeMPI<T>::getType();
  auto mpi_type_str = TypeMPI<T>::getTypeStr();
  RequestHolder r;
  if (mpi2_) {
    r.add([=]{
      LockMPI _scope_lock(l, node, data_window_);
      vt_debug_print(
        verbose, rdma,
        "MPI_Accumulate({}, {}, {}, {}, {}, {}, {}, window);\n",
        print_ptr(ptr), len, mpi_type_str, node, offset, len, mpi_type_str
      );
      VT_ALLOW_MPI_CALLS;
      MPI_Accumulate(
        ptr, len, mpi_type, node, offset, len, mpi_type, op, data_window_
      );
    });
  } else {
    LockMPI _scope_lock(l, node, data_window_);
    vt_debug_print(
      verbose, rdma,
      "MPI_Raccumulate({}, {}, {}, {}, {}, {}, {}, window);\n",
      print_ptr(ptr), len, mpi_type_str, node, offset, len, mpi_type_str
    );
    VT_ALLOW_MPI_CALLS;
    MPI_Raccumulate(
      ptr, len, mpi_type, node, offset, len, mpi_type, op, data_window_, r.add()
    );
  }
  return r;
}

template <typename T, HandleEnum E>
void Holder<T,E>::accum(
  vt::NodeType node, Lock l, T* ptr, std::size_t len, int offset,
  MPI_Op op
) {
  raccum(node, l, ptr, len, offset, op);
}

template <typename T, HandleEnum E>
void Holder<T,E>::fence(int assert) {
  VT_ALLOW_MPI_CALLS;
  MPI_Win_fence(assert, data_window_);
}

template <typename T, HandleEnum E>
void Holder<T,E>::sync() {
  VT_ALLOW_MPI_CALLS;
  MPI_Win_sync(data_window_);
}

template <typename T, HandleEnum E>
void Holder<T,E>::flush(vt::NodeType node) {
  VT_ALLOW_MPI_CALLS;
  MPI_Win_flush(node, data_window_);
}

template <typename T, HandleEnum E>
void Holder<T,E>::flushLocal(vt::NodeType node) {
  VT_ALLOW_MPI_CALLS;
  MPI_Win_flush_local(node, data_window_);
}

template <typename T, HandleEnum E>
void Holder<T,E>::flushAll() {
  VT_ALLOW_MPI_CALLS;
  MPI_Win_flush_all(data_window_);
}


}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HOLDER_IMPL_H*/
