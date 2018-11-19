
#include "vt/transport.h"

#include <vector>

namespace vt { namespace tutorial {

/*
 * Some user defined data structures
 */
struct Particle {
  Particle() = default;
  Particle(double in_x, double in_y, double in_z)
    : x(in_x), y(in_y), z(in_z)
  { }

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

//                  VT Base Message
//                 \----------------/
//                  \              /
struct ParticleMsg : ::vt::Message {
  ParticleMsg() = default;

  ParticleMsg(int in_x, int in_y, int in_z)
    : x(in_x), y(in_y), z(in_z)
  { }

  /*
   * Implement a serialize method so the std::vector and pointer are properly
   * serialization on the send
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
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

  /*
   * The theMsg()->sendMsgAuto(..) will serialize the message sent to the
   * destination node if it has a serialize method. If not, it will send the
   * message as if it is sent directly the sendMsg.
   */

  if (this_node == 0) {
    NodeType const to_node = 1;
    auto msg = ::vt::makeSharedMessage<ParticleMsg>(1,2,3);
    msg->particles.push_back(Particle{10,11,12});
    msg->particles.push_back(Particle{13,14,15});
    ::vt::theMsg()->sendMsgAuto<ParticleMsg,msgSerialA>(to_node, msg);
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

}} /* end namespace vt::tutorial */
