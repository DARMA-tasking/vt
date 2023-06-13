/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_2b.h
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

#include "vt/transport.h"

namespace vt { namespace tutorial {

/// [Tutorial2B]
// Forward declaration for message
struct ColRedMsg;

//                  VT Base Class for a collection
//            \------------------------------------------/
//             \                                        /
//              \                              Index   /
//               \                          \---------/
//                \                          \       /
struct ReduceCol : ::vt::Collection<ReduceCol,Index1D> {

  void reduceHandler();
  void reduceTarget(int value);

};

//                  VT Base Message for Collections
//               \-----------------------------------/
//                \                                 /
struct ColRedMsg : ::vt::CollectionMessage<ReduceCol> { };

void ReduceCol::reduceHandler() {
  auto cur_node = theContext()->getNode();
  (void)cur_node;  // don't warn about unused variable
  auto idx = this->getIndex();
  (void)idx;  // don't warn about unused variable

  //::fmt::print("MyCol::reduceHandler index={}, node={}\n", idx.x(), cur_node);

  auto proxy = getCollectionProxy();
  int val = 100;

  // Invoke the reduce!
  proxy.allreduce<&ReduceCol::reduceTarget, collective::PlusOp>(val);
}

int number_of_elements = 32;

void ReduceCol::reduceTarget(int value) {
  fmt::print("collection reduce value={}\n", value);
  assert(number_of_elements * 100 == value);

}

// Tutorial code to demonstrate reducing a collection
static inline void collectionReduce() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * This is an example of reducing over a virtual context collection
   */

  if (this_node == 0) {
    // Range of `number_of_elements` elements for the collection
    auto range = vt::Index1D(number_of_elements);

    // Construct the collection in a rooted manner. By default, the elements
    // will be block mapped to the nodes
    auto proxy = vt::makeCollectionRooted<ReduceCol>("tutorial_collection_reduce")
      .bounds(range)     // Set the bounds for the collection
      .bulkInsert()      // Bulk insert all the elements within the bounds
      .wait();           // Wait for construction and get the proxy back

    // Broadcast a message to the entire collection. The reduceHandler will be
    // invoked on every element to the collection
    proxy.broadcast<&ReduceCol::reduceHandler>();
  }
}
/// [Tutorial2B]

}} /* end namespace vt::tutorial */
