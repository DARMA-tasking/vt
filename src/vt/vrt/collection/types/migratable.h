/*
//@HEADER
// *****************************************************************************
//
//                                 migratable.h
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

#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/base/base.h"
#include "vt/vrt/collection/types/migrate_hooks.h"
#include "vt/vrt/collection/types/migratable.fwd.h"
#include "vt/vrt/collection/types/storage/storable.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/elm_stats.h"

namespace vt { namespace vrt { namespace collection {

// Forward declaration for friend declaration below
namespace balance {
  struct NodeStats;
}

struct Migratable : MigrateHookBase, storage::Storable {

  Migratable();

  /*
   * The user or runtime system can invoke this method at any time (when a valid
   * pointer to it exists) to migrate this VCC element to another memory domain
   *
   *  1.  Invoke migrate(node) where node != theContext()->getNode()
   *  2.  Runtime system invokes Migratable::preMigrateOut()
   *  3.  Migratable element is serialized
   *  4.  Migratable element is sent to the destination node
   *  5.  Location manager de-registers this element
   *  6.  VCC manager removes local reference to this element
   *  7.  Runtime system invokes Migratable::epiMigrateOut()
   *  8.  Runtime system invokes Migratable::destroy()
   *  9.  Runtime system invokes the destructor
   *      ....... send MigrateMsg to MigrateHandlers::migrateInHandler .......
   *  10. migrateInHandler() is invoked
   *  11. Migratable element is de-serialized
   *  12. Migratable element constructed with migration constructor
   *  13. Runtime system invokes Migratable::preMigrateIn()
   *  14. VCC manager add local reference to this element (InnerHolder)
   *  15. Location manager registers this element on destination node
   *  16. Runtime system invokes Migratable::epiMigrateIn()
   *
   */
  /*
    @todo: migrate interface is through base class HasMigrate to insert lower in
           the hierarchy
     virtual void migrate(NodeType const& node);
  */

  /*
   * The system invokes this when the destructor is about to be called on the
   * VCC element due a migrate(NodeType const&) invocation
   */
  virtual void destroy();

  balance::ElementIDType getElmID() const { return stats_elm_id_; }
  balance::ElementIDType getTempID() const { return temp_elm_id_; }

protected:
  template <typename Serializer>
  void serialize(Serializer& s);

protected:
  friend struct balance::ElementStats;
  friend struct balance::NodeStats;
  balance::ElementStats stats_;
public:
  balance::ElementStats& getStats() { return stats_; }
protected:
  balance::ElementIDType stats_elm_id_ = 0;
  balance::ElementIDType temp_elm_id_ = 0;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/types/migratable.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H*/
