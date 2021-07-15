/*
//@HEADER
// *****************************************************************************
//
//                             handle.index.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_RDMAHANDLE_HANDLE_INDEX_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_HANDLE_INDEX_IMPL_H

#include "vt/config.h"

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename I>
bool Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::ready() const {
  if (proxy_ != no_obj_group) {
    auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
    return proxy.get()->ready();
  } else {
    return false;
  }
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::readExclusive(
  std::function<void(T const*, std::size_t count)> fn
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  proxy.get()->access(index_, Lock::Exclusive, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::readShared(
  std::function<void(T const*, std::size_t count)> fn
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  proxy.get()->access(index_, Lock::Shared, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::modifyExclusive(
  std::function<void(T*, std::size_t count)> fn
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  proxy.get()->access(index_, Lock::Exclusive, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::modifyShared(
  std::function<void(T*, std::size_t count)> fn
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  proxy.get()->access(index_, Lock::Shared, fn, this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::lock(Lock l, I const& index) {
  //@todo: implement this
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::unlock() {
  this->lock_ = nullptr;
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::get(
  I const& index, T* ptr, std::size_t len, int offset, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  proxy.get()->get(index, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
typename Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::RequestType
Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::rget(
  I const& index, T* ptr, std::size_t len, int offset, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  return proxy.get()->rget(index, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::get(
  I const& index, std::size_t len, int offset, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  proxy.get()->rget(index, len, offset);
}

template <typename T, HandleEnum E, typename I>
typename Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::RequestType
Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::rget(
  I const& index, std::size_t len, int offset, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  if (this->getBuffer() == nullptr) {
    auto ptr = std::make_unique<T[]>(len);
    auto r = proxy.get()->rget(
      index, l, &ptr[0], len, offset + this->hoff()
    );
    r.addAction([cptr=std::move(ptr),actions=this->actions_]{
      for (auto&& action : actions) {
        action(&cptr[0]);
      }
    });
    return r;
  } else {
    auto r = proxy.get()->rget(
      index, l, this->user_buffer_, len, offset + this->hoff()
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
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::put(
  I const& index, T* ptr, std::size_t len, int offset, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  return proxy.get()->put(index, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
typename Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::RequestType
Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::rput(
  I const& index, T* ptr, std::size_t len, int offset, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  return proxy.get()->rput(index, l, ptr, len, offset + this->hoff());
}

template <typename T, HandleEnum E, typename I>
void Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::accum(
  I const& index, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  return proxy.get()->accum(index, l, ptr, len, offset + this->hoff(), op);
}

template <typename T, HandleEnum E, typename I>
typename Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::RequestType
Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::raccum(
  I const& index, T* ptr, std::size_t len, int offset, MPI_Op op, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  return proxy.get()->raccum(index, l, ptr, len, offset + this->hoff(), op);
}

template <typename T, HandleEnum E, typename I>
T Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::fetchOp(
  I const& index, T ptr, int offset, MPI_Op op, Lock l
) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  return proxy.get()->fetchOp(index, l, ptr, offset, op);
}

template <typename T, HandleEnum E, typename I>
std::size_t Handle<
  T,E,I,typename std::enable_if_t<not std::is_same<I,vt::NodeType>::value>
>::getCount(I const& index) {
  auto proxy = vt::objgroup::proxy::Proxy<SubHandle<T,E,I>>(proxy_);
  if (proxy.get()->isUniform()) {
    return this->count();
  } else {
    return proxy.get()->getCount(index);
  }
}


}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_HANDLE_INDEX_IMPL_H*/
