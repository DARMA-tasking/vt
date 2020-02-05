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
#include "vt/rdmahandle/sub_handle.h"

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename I>
template <typename U>
bool Handle<T,E,I>::ready(isNodeType<U>*) const {
  return vt::theHandle()->getEntry<T,E>(key_).ready();
}

template <typename T, HandleEnum E, typename I>
template <typename U>
bool Handle<T,E,I>::ready(isIndexType<U>*) const {
  if (proxy_ != no_obj_group) {
    auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
    return proxy.get()->ready();
  } else {
    return false;
  }
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::get(U node, T* ptr, std::size_t len, int offset, Lock l, isNodeType<U>*) {
  return vt::theHandle()->getEntry<T,E>(key_).get(node, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::rget(U node, T* ptr, std::size_t len, int offset, Lock l, isNodeType<U>*) {
  return vt::theHandle()->getEntry<T,E>(key_).rget(node, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::get(U node, std::size_t len, int offset, Lock l, isNodeType<U>*) {
  rget(node, len, offset);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::rget(U node, std::size_t len, int offset, Lock l, isNodeType<U>*) {
  if (getBuffer() == nullptr) {
    auto ptr = std::make_unique<T[]>(len);
    auto r = vt::theHandle()->getEntry<T,E>(key_).rget(
      node, l, &ptr[0], len, offset + hoff_
    );
    r.addAction([cptr=std::move(ptr),actions=actions_]{
      for (auto&& action : actions) {
        action(&cptr[0]);
      }
    });
    return r;
  } else {
    auto r = vt::theHandle()->getEntry<T,E>(key_).rget(
      node, l, user_buffer_, len, offset + hoff_
    );
    r.addAction([buffer=user_buffer_,actions=actions_]{
      for (auto&& action : actions) {
        action(buffer);
      }
    });
    return r;
  }
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::put(
  U node, T* ptr, std::size_t len, int offset, Lock l, isNodeType<U>*
) {
  return vt::theHandle()->getEntry<T,E>(key_).put(node, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::rput(U node, T* ptr, std::size_t len, int offset, Lock l, isNodeType<U>*) {
  return vt::theHandle()->getEntry<T,E>(key_).rput(node, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::accum(
  U node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l, isNodeType<U>*
) {
  return vt::theHandle()->getEntry<T,E>(key_).accum(node, l, ptr, len, offset + hoff_, op);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::raccum(
  U node, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l, isNodeType<U>*
) {
  return vt::theHandle()->getEntry<T,E>(key_).raccum(node, l, ptr, len, offset + hoff_, op);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
std::size_t Handle<T,E,I>::getSize(U node, isNodeType<U>*) {
  if (vt::theHandle()->getEntry<T,E>(key_).isUniform()) {
    return size_;
  } else {
    return vt::theHandle()->getEntry<T,E>(key_).getSize(node);
  }
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::readExclusive(std::function<void(T const*)> fn, isNodeType<U>*) {
  vt::theHandle()->getEntry<T,E>(key_).access(Lock::Exclusive, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::readShared(std::function<void(T const*)> fn, isNodeType<U>*) {
  vt::theHandle()->getEntry<T,E>(key_).access(Lock::Shared, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::modifyExclusive(std::function<void(T*)> fn, isNodeType<U>*) {
  vt::theHandle()->getEntry<T,E>(key_).access(Lock::Exclusive, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::modifyShared(std::function<void(T*)> fn, isNodeType<U>*) {
  vt::theHandle()->getEntry<T,E>(key_).access(Lock::Shared, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::access(
  Lock l, std::function<void(T*)> fn, std::size_t offset, isNodeType<U>*
) {
  vt::theHandle()->getEntry<T,E>(key_).access(l, fn, hoff_ +  offset);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::readExclusive(
  U idx, std::function<void(T const*)> fn, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  proxy.get()->access(index_, Lock::Exclusive, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::readShared(
  U idx, std::function<void(T const*)> fn, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  proxy.get()->access(index_, Lock::Shared, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::modifyExclusive(
  U idx, std::function<void(T*)> fn, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  proxy.get()->access(index_, Lock::Exclusive, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::modifyShared(
  U idx, std::function<void(T*)> fn, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  proxy.get()->access(index_, Lock::Shared, fn, hoff_);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I>::lock(Lock l, vt::NodeType node) {
  lock_ = vt::theHandle()->getEntry<T,E>(key_).lock(l, node);
}

template <typename T, HandleEnum E, typename I>
void Handle<T,E,I>::unlock() {
  lock_ = nullptr;
}

///////////////////////////////////////////////////////////////////////////
// Index Overloads
///////////////////////////////////////////////////////////////////////////

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::get(U index, T* ptr, std::size_t len, int offset, Lock l, isIndexType<U>*) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  proxy.get()->get(index, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::rget(U index, T* ptr, std::size_t len, int offset, Lock l, isIndexType<U>*) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  return proxy.get()->rget(index, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::get(U index, std::size_t len, int offset, Lock l, isIndexType<U>*) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  proxy.get()->rget(index, len, offset);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::rget(U index, std::size_t len, int offset, Lock l, isIndexType<U>*) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  if (getBuffer() == nullptr) {
    auto ptr = std::make_unique<T[]>(len);
    auto r = proxy.get()->rget(
      index, l, &ptr[0], len, offset + hoff_
    );
    r.addAction([cptr=std::move(ptr),actions=actions_]{
      for (auto&& action : actions) {
        action(&cptr[0]);
      }
    });
    return r;
  } else {
    auto r = proxy.get()->rget(
      index, l, user_buffer_, len, offset + hoff_
    );
    r.addAction([buffer=user_buffer_,actions=actions_]{
      for (auto&& action : actions) {
        action(buffer);
      }
    });
    return r;
  }
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::put(
  U index, T* ptr, std::size_t len, int offset, Lock l, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  return proxy.get()->put(index, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::rput(U index, T* ptr, std::size_t len, int offset, Lock l, isIndexType<U>*) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  return proxy.get()->rput(index, l, ptr, len, offset + hoff_);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
void Handle<T,E,I>::accum(
  U index, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  return proxy.get()->accum(index, l, ptr, len, offset + hoff_, op);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
typename Handle<T,E,I>::RequestType
Handle<T,E,I>::raccum(
  U index, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l, isIndexType<U>*
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  return proxy.get()->raccum(index, l, ptr, len, offset + hoff_, op);
}

template <typename T, HandleEnum E, typename I>
template <typename U>
std::size_t Handle<T,E,I>::getSize(U node, isIndexType<U>*) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,U>>(proxy_);
  if (proxy.get()->isUniform()) {
    return size_;
  } else {
    return proxy.get()->getSize(node);
  }
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_IMPL_H*/
