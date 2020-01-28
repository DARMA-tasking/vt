/*
//@HEADER
// *****************************************************************************
//
//                                  manager.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_MANAGER_H
#define INCLUDED_VT_RDMAHANDLE_MANAGER_H

#include "vt/config.h"
#include "vt/rdmahandle/handle.h"
#include "vt/rdmahandle/handle_key.h"
#include "vt/rdmahandle/type_mpi.h"
#include "vt/objgroup/manager.h"
#include "vt/pipe/pipe_manager.h"

namespace vt { namespace rdma {

using ElemType = int64_t;

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
  template <typename Callable>
  void access(bool exclusive, Callable fn) {
    auto this_node = theContext()->getNode();
    auto lock_type = exclusive ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED;
    MPI_Win_lock(lock_type, this_node, 0, data_window_);
    fn(data_base_);
    MPI_Win_unlock(this_node, data_window_);
  }

  struct RequestHolder {
    RequestHolder() = default;
    RequestHolder(RequestHolder const&) = delete;
    RequestHolder(RequestHolder&&) = default;

    ~RequestHolder() { wait(); }

  public:
    MPI_Request* add() {
      reqs_.emplace_back(MPI_Request{});
      return &reqs_[reqs_.size()-1];
    }

    bool done() const { return reqs_.size() == 0; }

    void wait() {
      if (done()) {
        return;
      } else {
        std::vector<MPI_Status> stats;
        stats.resize(reqs_.size());
        MPI_Waitall(reqs_.size(), &reqs_[0], &stats[0]);
        reqs_.clear();
      }
    }

  private:
    std::vector<MPI_Request> reqs_;
    bool finished_ = true;
  };

  void get(vt::NodeType node, bool exclusive, T* ptr, std::size_t len, int offset) {
    auto lock_type = exclusive ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED;
    auto mpi_type = TypeMPI<T>::getType();
    RequestHolder r;
    MPI_Win_lock(lock_type, node, 0, data_window_);
    MPI_Rget(ptr, len, mpi_type, node, offset, len, mpi_type, data_window_, r.add());
    MPI_Win_unlock(node, data_window_);
  }

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

struct Manager {
  using ProxyType    = vt::objgroup::proxy::Proxy<Manager>;
  using ElemToHandle = std::unordered_map<int64_t, HandleType>;

  Manager() = default;

private:
  void initialize(ProxyType in_proxy) {
    proxy_ = in_proxy;
  }

private:
  struct HandleData {
    HandleData(HandleKey in_key, std::size_t in_size, int in_count)
      : key_(in_key),
        size_(in_size),
        count_(in_count)
    { }

    friend HandleData operator+(HandleData a1, HandleData const& a2) {
      vtAssertExpr(a1.key_.handle_ != vt::no_handle);
      vtAssertExpr(a1.key_.handle_ == a2.key_.handle_);
      vtAssertExpr(a1.key_.isObjGroup() == a2.key_.isObjGroup());
      a1.size_ += a2.size_;
      a1.count_ += a2.count_;
      return a1;
    }

    HandleKey key_;
    std::size_t size_ = 0;
    int count_ = 0;
  };

  template <typename T, HandleEnum E, typename ProxyT>
  struct ConstructMsg : vt::collective::ReduceTMsg<HandleData> {
    ConstructMsg() = default;
    explicit ConstructMsg(HandleData&& data)
      : vt::collective::ReduceTMsg<HandleData>(std::move(data))
    { }
  };

  template <typename T, HandleEnum E, typename ProxyT>
  void finishMake(ConstructMsg<T, E, ProxyT>* msg) {
    auto const& key = msg->getVal().key_;
    auto const& size = msg->getVal().size_;
    auto const& count = msg->getVal().count_;
    fmt::print(
      "{}: finishMake: handle={:x}, size={}, count={}\n",
      theContext()->getNode(), key.handle_, size, count
    );
    auto& entry = getEntry<T,E>(key);
    entry.allocateDataWindow();
  }

public:
  template <typename T, HandleEnum E, typename ProxyT>
  Handle<T, E> makeHandleCollectiveObjGroup(ProxyT proxy, std::size_t size) {
    auto proxy_bits = proxy.getProxy();
    ElemType lin = 0;
    auto next_handle = ++cur_handle_obj_group_[proxy_bits];
    auto key = HandleKey{typename HandleKey::ObjGroupTag{}, proxy_bits, next_handle};
    auto han = Handle<T, E>(key, size);
    holder_<T,E>[key].template addHandle<ObjGroupProxyType>(key, lin, han, size);
    auto cb = vt::theCB()->makeBcast<
      Manager, ConstructMsg<T, E, ProxyT>, &Manager::finishMake<T, E, ProxyT>
    >(proxy_);
    auto msg = vt::makeMessage<ConstructMsg<T, E, ProxyT>>(HandleData{key, size, 1});
    proxy_.template reduce<vt::collective::PlusOp<HandleData>>(msg.get(),cb);
    return han;
  }

  template <
    typename T,
    HandleEnum E,
    typename ProxyT,
    typename IndexType = typename ProxyT::IndexType
  >
  Handle<T, E> makeHandleCollectiveCollection(
    ProxyT proxy, IndexType range, std::size_t size
  ) {
    auto proxy_bits = proxy.getProxy();
    auto idx = proxy.getIndex();
    auto lin = static_cast<ElemType>(mapping::linearizeDenseIndexRowMajor(&idx, &range));
    auto next_handle = ++cur_handle_collection_[proxy_bits][lin];
    auto key = HandleKey{typename HandleKey::CollectionTag{}, proxy_bits, next_handle, size};
    auto han = Handle<T, E>(key, size);
    holder_<T,E>[key].template addHandle<VirtualProxyType>(key, lin, han);
    return han;
  }

  // For now, this is static. Really it should be part of the objgroup and the
  // proxy should be available through the VT component
  template <typename T, HandleEnum E>
  static Holder<T,E>& getEntry(HandleKey const& key) {
    vtAssertExpr((holder_<T,E>.find(key) != holder_<T,E>.end()));
    auto& entry = holder_<T,E>[key];
    return entry;
  }

public:
  static ProxyType construct() {
    auto proxy = vt::theObjGroup()->makeCollective<Manager>();
    proxy.get()->initialize(proxy);
    return proxy;
  }

private:
  // Current collective handle for a given objgroup proxy
  std::unordered_map<ObjGroupProxyType, HandleType> cur_handle_obj_group_;

  // Current collective handle for a given collection proxy & element index
  std::unordered_map<VirtualProxyType, ElemToHandle> cur_handle_collection_;

  // Objgroup proxy for this manager
  ProxyType proxy_;

  // Holder for RDMA control data
  template <typename T, HandleEnum E>
  static std::unordered_map<HandleKey, Holder<T,E>> holder_;
};

template <typename T, HandleEnum E>
/*static*/ std::unordered_map<HandleKey, Holder<T,E>> Manager::holder_ = {};


}} /* end namespace vt::rdma */

namespace std {

template <>
struct hash<vt::rdma::HandleKey> {
  size_t operator()(vt::rdma::HandleKey const& in) const {
    return std::hash<uint64_t>()(
      in.handle_ ^
      (in.proxy_.is_obj_ ? in.proxy_.u_.obj_ : in.proxy_.u_.vrt_) ^
      (in.proxy_.is_obj_ ? 0x10 : 0x00)
    );
  }
};

} /* end namespace std */

#include "vt/rdmahandle/handle.impl.h"

#endif /*INCLUDED_VT_RDMAHANDLE_MANAGER_H*/
