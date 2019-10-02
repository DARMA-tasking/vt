/*
//@HEADER
// *****************************************************************************
//
//                          hello_world_vrtContext.cc
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

#include <cstdlib>
#include <iostream>

#include "vt/transport.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/vrt/context/context_vrtmanager.h"

using namespace vt;
using namespace vt::vrt;

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
      : Message(), from(in_from) {}
};

struct HelloVrtContext : VirtualContext {
  int from;

  explicit HelloVrtContext(int const& in_from)
      : VirtualContext(), from(in_from) {}
};

static void hello_world(HelloMsg *msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
}

int main(int argc, char **argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }



  if (my_node == 1) {

    auto vrtc1 = theVirtualManager()->makeVirtual<HelloVrtContext>(10);
    (void)vrtc1;
    auto vrtc2 = theVirtualManager()->makeVirtual<HelloVrtContext>(20);
    (void)vrtc2;

    // auto temp1 = theVrtCManager->getVrtContextByID(vrtc1);
    // auto hello1 = static_cast<HelloVrtContext*>(temp1);

    // theVrtCManager->destroyVrtContextByID(vrtc1);
    // auto temp1_ = theVrtCManager->getVrtContextByID(vrtc1);
    // auto hello1_ = static_cast<HelloVrtContext*>(temp1);

    // if (temp1_ != nullptr) {
    //   std::cout << "Boom "<<std::endl;
    // }

    // auto temp2 = theVrtCManager->getVrtContextByID(vrtc2);
    // auto hello2 = static_cast<HelloVrtContext*>(temp2);

    // if (temp2 != nullptr) {
    //   std::cout << "Boom "<<std::endl;
    // }


//    std::cout <<
//
//    std::cout << vrtc1 << std::endl;
//
//    VrtContext_IdType id1 = 5;
//    VrtContext_IdType id2 = 0;
//    auto temp1 = theVrtCManager->getVrtContextByID(id1);
////    auto temp2 = reinterpret_cast<HelloVrtContext*>(theVrtCManager->find(id2));
//
//    if (temp1 == nullptr) {
//      std::cout << "Cool" << std::endl;
//    }
//
//    auto temping = theVrtCManager->getVrtContextByID(id2);
//
//    auto temp2 = static_cast<HelloVrtContext*>(temping);
//
////    if (temp2 != nullptr) {
//      std::cout << temp2->getVrtContextNode() << std::endl;
////    }
//
//    auto vrtc1 = theVrtCManager->newVrtContext();

//    vrtc.printVrtContext();
//    vrtc1.printVrtContext();
//
//    HelloVrtContext my_vrtc(20);
//
//    my_vrtc.printVrtContext();
//
//    auto myHelloVrtC_proxy =
//        theVrtCManager->newVrtContext<HelloVrtContext>(&my_vrtc);
//
//    my_vrtc.printVrtContext();
//    std::cout << my_vrtc.from << std::endl;


//    my_vrtc.printVrtContext();
//    theVrtCManager->newVrtContext(my_vrtc);
//    my_vrtc.printVrtContext();
//    vrtc.printVrtContext();
//    vrtc1.printVrtContext();
////    std::cout << "My node: " << vrtc.getVrtContextNode() << std::endl;
////    std::cout << theVrtCManager_->newVrtContext() << std::endl;
//
//    std::cout << my_vrtc.getVrtContextUId() << std::endl;
//    std::cout << my_vrtc.isCollection() << std::endl;

    auto msg = makeSharedMessage<HelloMsg>(my_node);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
