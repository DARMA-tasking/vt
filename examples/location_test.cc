/*
//@HEADER
// ************************************************************************
//
//                          location_test.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

using EntityType = int32_t;

struct EntityMsg : vt::Message {
  EntityType entity;
  NodeType home;

  EntityMsg(EntityType const& in_entity, NodeType const& in_home)
    : Message(), entity(in_entity), home(in_home)
  { }
};

static void entityTestHandler(EntityMsg* msg) {
  fmt::print(
    "{}: entityTestHandler entity={}\n", theContext()->getNode(), msg->entity
  );

  theLocMan()->virtual_loc->getLocation(msg->entity, msg->home, [](NodeType node){
    fmt::print("{}: entityTestHandler: location={}\n", theContext()->getNode(), node);
    theLocMan()->virtual_loc->printCurrentCache();
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  int32_t entity = 10, entity2 = 11, entity3 = 12;

  if (my_node == 0) {
    theLocMan()->virtual_loc->registerEntity(entity,my_node);
    theLocMan()->virtual_loc->registerEntity(entity2,my_node);
  } else if (my_node == 1) {
    theLocMan()->virtual_loc->registerEntity(entity3,my_node);
  }

  if (my_node == 0) {
    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity, my_node)
    );
    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity2, my_node)
    );
  } else if (my_node == 1) {
    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity3, my_node)
    );
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
