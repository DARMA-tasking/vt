/*
//@HEADER
// *****************************************************************************
//
//                                  manager.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_MANAGER_H
#define INCLUDED_VT_TOPOS_LOCATION_MANAGER_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.fwd.h"
#include "vt/topos/location/manager.fwd.h"
#include "vt/topos/location/utility/coord.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/runtime/component/component_pack.h"

#include <unordered_map>
#include <functional>

namespace vt { namespace location {

/**
 * \struct LocationManager
 *
 * \brief A core VT component that manages instances of location managers for
 * specific virtual entity instances across a distributed machine.
 *
 * Tracks multiple instances of \c EntityLocationCoord which is the actual
 * manager for a common instance of distributed entities (like a collection that
 * is spread out across nodes). The \c LocationManager manages those instances
 * based on instance identifiers with a consistent mapping across the
 * distributed system.
 *
 * The location manager creates these coordinators on demand, because messages
 * might arrive out-of-order wrt the construction of the location coordinator.
 *
 */
struct LocationManager : runtime::component::Component<LocationManager> {
  template <typename LocType>
  using PtrType = LocType*;
  using LocCoordPtrType = LocationCoord*;
  using LocInstContainerType = std::unordered_map<LocInstType, LocCoordPtrType>;
  using VrtLocType = EntityLocationCoord<int32_t>;
  using VrtLocProxyType = EntityLocationCoord<VirtualProxyType>;

  template <typename IndexT>
  using IndexedElmType = EntityLocationCoord<IndexT>;

  template <typename IndexT>
  using CollectionLocType = PtrType<IndexT>;

  using CollectionContainerType = std::unordered_map<
    VirtualProxyType, ObjGroupProxyType
  >;

  /**
   * \internal \brief System call to construct a location manager
   */
  LocationManager() = default;

  virtual ~LocationManager();

  std::string name() override { return "LocationManager"; }

  void initialize() override;

  /**
   * \internal \brief Make a new location coordinator for a collection
   *
   * \param[in] proxy the collection proxy bits
   *
   * \return proxy to location manager
   */
  template <typename IndexT>
  objgroup::proxy::Proxy<IndexedElmType<IndexT>> makeCollectionLM(
    VirtualProxyType proxy
  );

  /**
   * \internal \brief Get the new location coordinator for a collection
   *
   * \param[in] proxy the collection proxy bits
   *
   * \return the location manager
   */
  template <typename IndexT>
  IndexedElmType<IndexT>* getCollectionLM(VirtualProxyType proxy);

  /**
   * \brief Destroy a location manager
   *
   * \param[in] proxy the collection proxy bits
   */
  template <typename IndexT>
  void destroyCollectionLM(VirtualProxyType proxy);

public:
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | collection_lms
      | virtual_loc
      | vrtContextLoc;
  }

public:
  PtrType<VrtLocType> virtual_loc;
  PtrType<VrtLocProxyType> vrtContextLoc;

private:
  CollectionContainerType collection_lms;
};

}} /* end namespace vt::location */

#include "vt/topos/location/manager.impl.h"

namespace vt {

extern location::LocationManager* theLocMan();

}  // end namespace vt

#endif /*INCLUDED_VT_TOPOS_LOCATION_MANAGER_H*/
