/*
//@HEADER
// *****************************************************************************
//
//                                  entity.cc
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

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/lb/instrumentation/entity.h"

#include <cassert>

namespace vt { namespace lb { namespace instrumentation {

/*static*/ LBEntityType Entity::cur_entity_id = 0;
/*static*/ std::unordered_map<LBEntityType, TimeType> Entity::events_;
/*static*/ std::unordered_map<LBEntityType, Entity::DatabaseType> Entity::entities_;
/*static*/ std::unordered_map<LBEntityType, Entity::MigratableType*> Entity::migratables_;

/*static*/ LBEntityType Entity::registerEntity() {
  auto const& node = theContext()->getNode();
  auto const& next_id = cur_entity_id++;
  return next_id | (static_cast<LBEntityType>(node) << 48);
}

/*static*/ LBEntityType Entity::registerMigratableEntity(MigratableType* mig) {
  auto const& entity = registerEntity();
  migratables_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(entity),
    std::forward_as_tuple(mig)
  );
  return entity;
}

/*static*/ void Entity::notifyMigrate(
  NodeType const& to_node, LBEntityType const& entity
) {
  auto iter = migratables_.find(entity);
  vtAssert(
    iter != migratables_.end(), "Entity must exist in migratables to migrate"
  );
  auto const& elm = iter->second;
  elm->migrate(to_node);
  migratables_.erase(iter);
}

/*static*/ void Entity::beginExecution(LBEntityType const& entity) {
  auto const& current_time = timing::Timing::getCurrentTime();
  auto event_iter = events_.find(entity);
  if (event_iter == events_.end()) {
    events_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(entity),
      std::forward_as_tuple(current_time)
    );
  } else {
    event_iter->second = current_time;
  }
}

/*static*/ void Entity::endExecution(LBEntityType const& entity) {
  auto entity_iter = entities_.find(entity);
  if (entity_iter == entities_.end()) {
    entities_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(entity),
      std::forward_as_tuple(DatabaseType{})
    );
    entity_iter = entities_.find(entity);
  }
  auto const& event_iter = events_.find(entity);
  vtAssert(
    event_iter != events_.end(), "Must have begin"
  );
  auto const& begin_time = event_iter->second;
  auto const& end_time = timing::Timing::getCurrentTime();
  entity_iter->second.addEntry(Entry{begin_time,end_time});
}

}}} /* end namespace vt::lb::instrumentation */
