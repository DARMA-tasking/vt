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

#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.fwd.h"
#include "vt/topos/location/manager.fwd.h"
#include "vt/topos/location/utility/coord.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy.h"
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
  checkpoint_virtual_serialize_derived_from(Component)

  template <typename LocType>
  using PtrType = std::unique_ptr<LocType>;
  using LocCoordPtrType = LocationCoord*;
  using LocInstContainerType = std::unordered_map<LocInstType, LocCoordPtrType>;
  using VrtLocType = EntityLocationCoord<int32_t>;
  using VrtLocProxyType = EntityLocationCoord<VirtualProxyType>;

  template <typename ColT, typename IndexT>
  using CollectionProxyType = ::vt::vrt::VirtualElmProxyType<ColT, IndexT>;
  template <typename ColT, typename IndexT>
  using VrtColl = EntityLocationCoord<CollectionProxyType<ColT, IndexT>>;
  template <typename ColT, typename IndexT>
  using CollectionLocType = PtrType<VrtColl<ColT, IndexT>>;
  using LocErasureType = LocationCoord;
  using LocDeleterType = std::function<void(LocErasureType*)>;
  using CollectionLocErasedType = std::unique_ptr<
    LocErasureType, LocDeleterType
  >;
  using CollectionContainerType = std::unordered_map<
    VirtualProxyType, CollectionLocErasedType
  >;
  template <typename LocType>
  using ActionLocInstType = std::function<void(LocType*)>;
  template <typename LocType>
  using PendingContainerType = std::vector<ActionLocInstType<LocType>>;

  /**
   * \internal \brief System call to construct a location manager
   */
  LocationManager() = default;

  virtual ~LocationManager();

  std::string name() override { return "LocationManager"; }

  /**
   * \internal \brief Next instance identifier
   */
  static LocInstType cur_loc_inst;

  PtrType<VrtLocType> virtual_loc = std::make_unique<VrtLocType>();
  PtrType<VrtLocProxyType> vrtContextLoc = std::make_unique<VrtLocProxyType>();

  /**
   * \internal \brief Get the location coordinator for a collection
   *
   * \param[in] proxy the collection proxy bits
   *
   * \return pointer to the coordinator, typed on the collection/index
   */
  template <typename ColT, typename IndexT>
  VrtColl<ColT, IndexT>* getCollectionLM(VirtualProxyType const& proxy);

  /**
   * \internal \brief Insert a new location coordinator for a collection
   *
   * \param[in] proxy the collection proxy bits
   */
  template <typename ColT, typename IndexT>
  void insertCollectionLM(VirtualProxyType const& proxy);

public:
  // Manage different instances of individually managed entities

  /**
   * \internal \brief Insert a new coordinator instance
   *
   * \param[in] i the instance ID
   * \param[in] ptr pointer to the coordinator
   */
  template <typename LocType>
  static void insertInstance(LocInstType const i, LocType* ptr);

  /**
   * \internal \brief Get a coordinator instance
   *
   * \param[in] inst the instance ID
   *
   * \return the location coordinator
   */
  static LocCoordPtrType getInstance(LocInstType const inst);

  /**
   * \internal \brief Perform an action on a coordinator
   *
   * \param[in] inst the instance ID
   * \param[in] action action to perform
   */
  template <typename LocType>
  static void applyInstance(
    LocInstType const inst, ActionLocInstType<LocType> action
  );

  template <
    typename SerializerT,
    typename = std::enable_if_t<
      std::is_same<SerializerT, checkpoint::Footprinter>::value
    >
  >
  void serialize(SerializerT& s) {
    s | collectionLoc
      | cur_loc_inst
      | loc_insts
      | virtual_loc
      | vrtContextLoc;
  }

protected:
  CollectionContainerType collectionLoc;

private:
  static LocInstContainerType loc_insts;
};

namespace details {

/**
 * \struct PendingInst
 *
 * \brief Hold templated static instances for location coordinators
 *
 * This is split out into a separate class to address the inability of
 * Intel 18 to process static member variable templates
 */
template <typename LocType>
struct PendingInst
{
  static std::unordered_map<
    LocInstType, LocationManager::PendingContainerType<LocType>
  > pending_inst_;
};

} /* end namespace details */

}} /* end namespace vt::location */

#include "vt/topos/location/manager.impl.h"

namespace vt {

extern location::LocationManager* theLocMan();

}  // end namespace vt

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_H*/
