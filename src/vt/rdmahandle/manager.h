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
#include "vt/rdmahandle/common.h"
#include "vt/rdmahandle/handle.h"
#include "vt/rdmahandle/handle_key.h"
#include "vt/rdmahandle/type_mpi.h"
#include "vt/rdmahandle/holder.h"
#include "vt/rdmahandle/manager.fwd.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/topos/mapping/dense/dense.h"
#include "vt/rdmahandle/handle_set.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/collective/collective_scope.h"

namespace vt { namespace rdma {

/** \file */

namespace impl {

struct HandleData;

template <typename T, HandleEnum E, typename ProxyT>
struct ConstructMsg;

template <typename ProxyT, typename IndexT>
struct InformRDMAMsg;

} /* end namespace impl */

/**
 * \struct Manager manager.h vt/rdmahandle/manager.h
 *
 * \brief RDMA Handle Manager for creation of node- or index-level handles
 */
struct Manager : runtime::component::Component<Manager> {
  checkpoint_virtual_serialize_derived_from(Component)

  using ProxyType       = vt::objgroup::proxy::Proxy<Manager>;
  using ElemToHandle    = std::unordered_map<int64_t, RDMA_HandleType>;
  using HandleToManager = std::unordered_map<RDMA_HandleType, ObjGroupProxyType>;
  using CollectiveScopeType = collective::CollectiveScope;

  Manager();

  std::string name() override { return "HandleRDMA"; }

  /**
   * \brief Destroy the component, called when VT is finalized
   */
  void finalize() override;

private:
  /**
   * \internal \brief Setup the manager with the objgroup proxy
   *
   * \param[in] in_proxy the manager instance's proxy
   */
  void setup(ProxyType in_proxy);

  /**
   * \internal \brief Finish constructing a handle after coordinating each node on
   * construction
   *
   * \param[in] msg construction meta-data message
   */
  template <typename T, HandleEnum E, typename ProxyT>
  void finishMake(impl::ConstructMsg<T, E, ProxyT>* msg);

  /**
   * \internal \brief Inform all nodes that an RDMA handle is being constructed,
   * required when a collection is not mapped to all nodes
   *
   * \param[in] msg inform msg
   */
  template <typename T, HandleEnum E, typename ProxyT, typename ColT>
  void informCollectionRDMA(
    impl::InformRDMAMsg<ProxyT, typename ColT::IndexType>* msg
  );

public:
  /**
   * \brief Construct a new, distributed RDMA handle for an objgroup
   *
   * \param[in] proxy the objgroup's proxy
   * \param[in] count the local count of T for this handle
   * \param[in] uniform_count whether all handles have the same count
   *
   * \return the new handle
   */
  template <typename T, HandleEnum E, typename ProxyT>
  Handle<T, E> makeHandleCollectiveObjGroup(
    ProxyT proxy, std::size_t count, bool uniform_count = true
  );

  /**
   * \brief Construct a static (non-migratable) set of new, distributed RDMA
   * handles for an objgroup
   *
   * \param[in] proxy the objgroup's proxy
   * \param[in] max_lookup the max lookup on any node
   * \param[in] map a map of the handles and corresponding sizes to create
   * \param[in] dense_start_with_zero handle indices are dense and start at zero
   * \param[in] uniform_size whether all handles have the same size
   *
   * \return the new handle set
   */
  template <
    typename T,
    HandleEnum E,
    typename ProxyT,
    typename LookupT = typename HandleSet<T>::LookupType
  >
  HandleSet<T> makeHandleSetCollectiveObjGroup(
    ProxyT proxy_objgroup,
    LookupT max_lookup,
    std::unordered_map<LookupT, std::size_t> const& map,
    bool dense_start_with_zero,
    bool uniform_size = true
  );

  /**
   * \brief Construct a migratable set of new, distributed RDMA
   * handles for a collection
   *
   * \param[in] collection_proxy the collection proxy with index
   * \param[in] idx_count the local count of T for this handle
   * \param[in] uniform_size whether all handles have the same size
   * \param[in] next_handle system-use-only, the handle ID
   * \param[in] map_han system-use-only, element map for collection
   * \param[in] in_range system-use-only, range for collection
   *
   * \return the new handle set
   */
  template <
    typename T,
    HandleEnum E,
    typename ColT,
    typename ProxyT,
    typename IndexT = typename ColT::IndexType
  >
  Handle<T, E, IndexT> makeCollectionHandles(
    ProxyT collection_proxy, std::size_t idx_count, bool uniform_size = true,
    RDMA_HandleType next_handle = no_rdma_handle, vt::HandlerType map_han = -1,
    IndexT in_range = {}
  );

  /**
   * \brief Destroy and garbage collect an RDMA handle
   *
   * \param[in] han the handle to destroy
   */
  template <typename T, HandleEnum E>
  void deleteHandleCollectiveObjGroup(Handle<T,E> const& han);

  /**
   * \brief Destroy and garbage collect an RDMA handle
   *
   * \param[in] han the handle to destroy
   */
  template <typename T>
  void deleteHandleSetCollectiveObjGroup(HandleSet<T>& han);

  /**
   * \brief Destroy and garbage collect a handle from a collection
   *
   * \param[in] han the handle to destroy
   */
  template <typename T, HandleEnum E, typename IndexT>
  void deleteHandleCollection(Handle<T,E,IndexT>& han);

  template <typename T, HandleEnum E>
  Holder<T,E>& getEntry(HandleKey const& key);

private:
  static vt::NodeType staticHandleMap(
    vt::Index2D* idx, vt::Index2D*, vt::NodeType
  ) {
    return static_cast<vt::NodeType>(idx->x());
  }

public:
  static std::unique_ptr<Manager> construct();

  template <
    typename SerializerT,
    typename = std::enable_if_t<
      std::is_same<SerializerT, checkpoint::Footprinter>::value
    >
  >
  void serialize(SerializerT& s) {
    s | cur_handle_obj_group_
      | cur_handle_collection_
      | collection_to_manager_
      | proxy_
      // holder_
      | collective_scope_;
  }

private:
  /// Current collective handle for a given objgroup proxy
  std::unordered_map<ObjGroupProxyType, RDMA_HandleType> cur_handle_obj_group_;

  /// Current collective handle for a given collection proxy & element index
  std::unordered_map<VirtualProxyType, ElemToHandle> cur_handle_collection_;

  /// The manager for a given handle and collection
  std::unordered_map<VirtualProxyType, HandleToManager> collection_to_manager_;

  /// Objgroup proxy for this manager
  ProxyType proxy_;

  /// Holder for RDMA control data
  template <typename T, HandleEnum E>
  static std::unordered_map<HandleKey, Holder<T,E>> holder_;

  // Collective scope for MPI operations
  CollectiveScopeType collective_scope_;
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
#include "vt/rdmahandle/manager.impl.h"

#endif /*INCLUDED_VT_RDMAHANDLE_MANAGER_H*/
