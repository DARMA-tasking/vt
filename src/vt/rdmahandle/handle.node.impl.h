/*
//@HEADER
// *****************************************************************************
//
//                              handle.node.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_NODE_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_NODE_IMPL_H

#include "vt/config.h"

namespace vt { namespace rdma {

/**
 * \brief Convenience using for when U is a vt::NodeType
 */
template <typename U>
using isNode = typename std::enable_if_t<std::is_same<U,vt::NodeType>::value>;

template <typename T, HandleEnum E, typename I>
bool Handle<T,E,I,isNode<I>>::ready() const {
  return vt::theHandle()->getEntry<T,E>(key_).ready();
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::get(
  vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l
) {
  return vt::theHandle()->getEntry<T,E>(key_).get(node, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
typename Handle<T,E,I,isNode<I>>::RequestType
Handle<T,E,I,isNode<I>>::rget(
  vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l
) {
    return vt::theHandle()->getEntry<T,E>(key_).rget(node, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::get(
  vt::NodeType node, std::size_t len, int offset, Lock l
) {
  rget(node, len, offset);
}

template <typename T, HandleEnum E, typename I>
typename Handle<T,E,I,isNode<I>>::RequestType
Handle<T,E,I,isNode<I>>::rget(vt::NodeType node, std::size_t len, int offset, Lock l) {
  if (this->getBuffer() == nullptr) {
    auto ptr = std::make_unique<T[]>(len);
    auto r = vt::theHandle()->getEntry<T,E>(key_).rget(
      node, l, &ptr[0], len, offset + this->hoff()
    );
    r.addAction([cptr=std::move(ptr),actions=this->actions_]{
      for (auto&& action : actions) {
        action(&cptr[0]);
      }
    });
    return r;
  } else {
    auto r = vt::theHandle()->getEntry<T,E>(key_).rget(
      node, l, this->user_buffer_, len, offset + this->hoff()
    );
    r.addAction([buffer=this->user_buffer_,actions=this->actions_]{
      for (auto&& action : actions) {
        action(buffer);
      }
    });
    return r;
  }
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::put(
  vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l
) {
  return vt::theHandle()->getEntry<T,E>(key_).put(node, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
typename Handle<T,E,I,isNode<I>>::RequestType
Handle<T,E,I,isNode<I>>::rput(
  vt::NodeType node, T* ptr, std::size_t len, int offset, Lock l
) {
  return vt::theHandle()->getEntry<T,E>(key_).rput(node, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::accum(
  vt::NodeType node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l
) {
  return vt::theHandle()->getEntry<T,E>(key_).accum(node, l, ptr, len, offset + this->hoff(), op);
}

template <typename T, HandleEnum E, typename I>
typename Handle<T,E,I,isNode<I>>::RequestType
Handle<T,E,I,isNode<I>>::raccum(
  vt::NodeType node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l
) {
  return vt::theHandle()->getEntry<T,E>(key_).raccum(node, l, ptr, len, offset + this->hoff(), op);
}

template <typename T, HandleEnum E, typename I>
T Handle<T,E,I,isNode<I>>::fetchOp(
  vt::NodeType node, T ptr, int offset, MPI_Op op, Lock l
) {
  return vt::theHandle()->getEntry<T,E>(key_).fetchOp(node, l, ptr, offset, op);
}

template <typename T, HandleEnum E, typename I>
std::size_t Handle<T,E,I,isNode<I>>::getSize(vt::NodeType node) {
  if (vt::theHandle()->getEntry<T,E>(key_).isUniform()) {
    return this->size();
  } else {
    return vt::theHandle()->getEntry<T,E>(key_).getSize(node);
  }
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::readExclusive(std::function<void(T const*)> fn) {
  vt::theHandle()->getEntry<T,E>(key_).access(Lock::Exclusive, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::readShared(std::function<void(T const*)> fn) {
    vt::theHandle()->getEntry<T,E>(key_).access(Lock::Shared, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::modifyExclusive(std::function<void(T*)> fn) {
    vt::theHandle()->getEntry<T,E>(key_).access(Lock::Exclusive, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::modifyShared(std::function<void(T*)> fn) {
    vt::theHandle()->getEntry<T,E>(key_).access(Lock::Shared, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::access(
  Lock l, std::function<void(T*)> fn, std::size_t offset
) {
    vt::theHandle()->getEntry<T,E>(key_).access(l, fn, this->hoff() +  offset);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::fence(int assert) {
  vt::theHandle()->getEntry<T,E>(key_).fence(assert);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::sync() {
  vt::theHandle()->getEntry<T,E>(key_).sync();
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::flush(vt::NodeType node) {
  vt::theHandle()->getEntry<T,E>(key_).flush(node);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::flushLocal(vt::NodeType node) {
  vt::theHandle()->getEntry<T,E>(key_).flushLocal(node);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::flushAll() {
  vt::theHandle()->getEntry<T,E>(key_).flushAll();
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::lock(Lock l, vt::NodeType node) {
  this->lock_ = vt::theHandle()->getEntry<T,E>(key_).lock(l, node);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I,isNode<I>>::unlock() {
  this->lock_ = nullptr;
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_NODE_IMPL_H*/
