/*
//@HEADER
// *****************************************************************************
//
//                                handle.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_IMPL_H

#include "vt/config.h"
#include "vt/rdmahandle/handle.h"
#include "vt/rdmahandle/holder.h"
#include "vt/rdmahandle/manager.h"

namespace vt { namespace rdma {

template <typename T, HandleEnum E>
bool Handle<T,E>::ready() const {
  return Manager::getEntry<T,E>(key_).ready();
}

template <typename T, HandleEnum E>
void Handle<T,E>::get(vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l) {
  return Manager::getEntry<T,E>(key_).get(node, l, ptr, len, offset);
}

template <typename T, HandleEnum E>
typename Handle<T,E>::RequestType
Handle<T,E>::rget(vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l) {
  return Manager::getEntry<T,E>(key_).rget(node, l, ptr, len, offset);
}

template <typename T, HandleEnum E>
void Handle<T,E>::get(vt::NodeType node, std::size_t len, int offset, Lock l) {
  rget(node, len, offset);
}

template <typename T, HandleEnum E>
typename Handle<T,E>::RequestType
Handle<T,E>::rget(vt::NodeType node, std::size_t len, int offset, Lock l) {
  if (getBuffer() == nullptr) {
    auto ptr = std::make_unique<T[]>(len);
    auto r = Manager::getEntry<T,E>(key_).rget(node, l, &ptr[0], len, offset);
    r.addAction([cptr=std::move(ptr),actions=actions_]{
      for (auto&& action : actions) {
        action(&cptr[0]);
      }
    });
    return r;
  } else {
    auto r = Manager::getEntry<T,E>(key_).rget(node, l, user_buffer_, len, offset);
    r.addAction([buffer=user_buffer_,actions=actions_]{
      for (auto&& action : actions) {
        action(buffer);
      }
    });
    return r;
  }
}

template <typename T, HandleEnum E>
void Handle<T,E>::put(vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l) {
  return Manager::getEntry<T,E>(key_).put(node, l, ptr, len, offset);
}

template <typename T, HandleEnum E>
typename Handle<T,E>::RequestType
Handle<T,E>::rput(vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l) {
  return Manager::getEntry<T,E>(key_).rput(node, l, ptr, len, offset);
}

template <typename T, HandleEnum E>
void Handle<T,E>::accum(
  vt::NodeType node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l
) {
  return Manager::getEntry<T,E>(key_).accum(node, l, ptr, len, offset, op);
}

template <typename T, HandleEnum E>
typename Handle<T,E>::RequestType
Handle<T,E>::raccum(
  vt::NodeType node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l
) {
  return Manager::getEntry<T,E>(key_).raccum(node, l, ptr, len, offset, op);
}

template <typename T, HandleEnum E>
void Handle<T,E>::readExclusive(std::function<void(T const*)> fn) {
  Manager::getEntry<T,E>(key_).access(Lock::Exclusive, fn);
}

template <typename T, HandleEnum E>
void Handle<T,E>::readShared(std::function<void(T const*)> fn) {
  Manager::getEntry<T,E>(key_).access(Lock::Shared, fn);
}

template <typename T, HandleEnum E>
void Handle<T,E>::modifyExclusive(std::function<void(T*)> fn) {
  Manager::getEntry<T,E>(key_).access(Lock::Exclusive, fn);
}

template <typename T, HandleEnum E>
void Handle<T,E>::modifyShared(std::function<void(T*)> fn) {
  Manager::getEntry<T,E>(key_).access(Lock::Shared, fn);
}

template <typename T, HandleEnum E>
void Handle<T,E>::lock(Lock l, vt::NodeType node) {
  lock_ = Manager::getEntry<T,E>(key_).lock(l, node);
}

template <typename T, HandleEnum E>
void Handle<T,E>::unlock() {
  lock_ = nullptr;
}


}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_IMPL_H*/
