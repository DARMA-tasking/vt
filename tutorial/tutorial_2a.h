/*
//@HEADER
// ************************************************************************
//
//                          tutorial_2a.h
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

namespace vt { namespace tutorial {

// Forward declaration for message
struct MyCollMsg;

//               VT Base Class for a collection
//         \-------------------------------------/
//          \                                   /
//           \                         Index   /
//            \                     \---------/
//             \                     \       /
struct MyCol : ::vt::Collection<MyCol,Index1D> {

  void msgHandler(MyCollMsg* msg);

};

//                 VT Base Message for Collections
//               \--------------------------------/
//                \                              /
struct MyCollMsg : ::vt::CollectionMessage<MyCol> { };

void MyCol::msgHandler(MyCollMsg* msg) {
  auto cur_node = theContext()->getNode();
  auto idx = this->getIndex();
  ::fmt::print("MyCol::msgHandler index={}, node={}\n", idx.x(), cur_node);
}

// Tutorial code to demonstrate creating a collection
static inline void collection() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * This is an example of creating a virtual context collection with an index
   * range
   */

  if (this_node == 0) {
    // Range of 32 elements for the collection
    auto range = ::vt::Index1D(32);
    // Construct the collection: invoked by one node. By default, the elements
    // will be block mapped to the nodes
    auto proxy = theCollection()->construct<MyCol>(range);

    // Broadcast a message to the entire collection. The msgHandler will be
    // invoked on every element to the collection
    auto msg = ::vt::makeSharedMessage<MyCollMsg>();
    proxy.broadcast<MyCollMsg,&MyCol::msgHandler>(msg);

    // Send a message to the 5th element of the collection
    auto msg2 = ::vt::makeSharedMessage<MyCollMsg>();
    proxy[5].send<MyCollMsg,&MyCol::msgHandler>(msg2);
  }
}

}} /* end namespace vt::tutorial */
