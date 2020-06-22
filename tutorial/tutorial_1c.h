/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1c.h
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

#include "vt/transport.h"

#include <vector>

namespace vt { namespace tutorial {

/// [Tutorial1C]
/*
 * Some user defined data structures
 */
struct Particle {
  Particle() = default;
  Particle(double in_x, double in_y, double in_z)
    : x(in_x), y(in_y), z(in_z)
  { }

  // Non-message types can use serializers like this.
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | x | y | z;
  }

  double x, y, z;
};

struct Data {
  int a;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | a;
  }
};

// TODO - show data with non-intrusive serialization
// TODO - show data that does not need serialization

//                  VT Base Message
//                 \----------------/
//                  \              /
struct ParticleMsg : ::vt::Message {
  using MessageParentType = ::vt::Message; // base message
  vt_msg_serialize_required();         // mark serialization mode

  ParticleMsg() = default;

  ParticleMsg(int in_x, int in_y, int in_z)
    : x(in_x), y(in_y), z(in_z)
  { }

  /*
   * Implement a serialize method so the std::vector and pointer are properly
   * serialization on the send.
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);    // ensure parent is serialized consistently

    s | particles;
    s | x | y | z;

    // Serialize a pointer by first serializing whether it is non-null and then
    // the actual value if it's not non-null
    bool has_a = a != nullptr;
    s | has_a;
    if (has_a) {
      s | *a;
    }
  }

public:
  std::vector<Particle> particles;
  int x = 0, y = 0, z = 0;
  Data* a = nullptr;
};

// Forward declaration for the active message handler
static void msgSerialA(ParticleMsg* msg);

// Tutorial code to demonstrate serialization in active message sends
static inline void activeMessageSerialization() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * The theMsg()->sendMsg(..) will serialize the message sent to the
   * destination node if it has a serialize method. If not, it will send the
   * message as if it is sent directly the sendMsg.
   */

  if (this_node == 0) {
    NodeType const to_node = 1;
    auto msg = ::vt::makeMessage<ParticleMsg>(1,2,3);
    msg->particles.push_back(Particle{10,11,12});
    msg->particles.push_back(Particle{13,14,15});
    ::vt::theMsg()->sendMsg<ParticleMsg,msgSerialA>(to_node, msg.get());
  }
}

// Message handler
static void msgSerialA(ParticleMsg* msg) {
  vtAssert(msg->particles.size() == 2, "Should be two particles");
  vtAssert(msg->x == 1 && msg->y == 2 && msg->z == 3, "Values x,y,z incorrect");

  auto const cur_node = ::vt::theContext()->getNode();

  /* Node 0 sends MyMsg to node 1 in the above code so this should execute on
   * node 1 */
  vtAssert(cur_node == 1, "This handler should execute on node 1");

  ::fmt::print("msgSerialA: triggered on node={}\n", cur_node);
}
/// [Tutorial1C]

}} /* end namespace vt::tutorial */
