
#include "vt/transport.h"

#include "tutorial_1a.h"
#include "tutorial_1b.h"
#include "tutorial_1c.h"
#include "tutorial_1d.h"
#include "tutorial_1e.h"
#include "tutorial_1f.h"
#include "tutorial_1g.h"
#include "tutorial_1h.h"

#include "tutorial_2a.h"
#include "tutorial_2b.h"

#include "tutorial_3a.h"

int main(int argc, char** argv) {
  using namespace vt::tutorial;

  /*
   * By default if VT is initialized without an MPI communicator, VT will
   * initialize MPI itself and finalize MPI in its finalize call
   */
  ::vt::CollectiveOps::initialize(argc, argv);

  /*
   * The tutorial pieces generate work for many nodes, but they all stem from
   * node 0. This is a common pattern. Collections and other virtualized
   * entities will be created at a given node. The collection may then be mapped
   * to other nodes, but the user may not need to be explicitly aware where it
   * is executing.
   *
   * For non-virtualized entities, such as a message handler running on a
   * particular node, these will typically stem from other invocations
   * transitively. Thus, often nodes that aren't involved in the immediate kick
   * off of work will spin in the scheduler awaiting a handler.
   */

  // These parts of the tutorial only require the root node
  if (::vt::theContext()->getNode() == 0) {
    // Invoke tutorial 1a: context (node, number of nodes)
    context();

    // Invoke tutorial 1b: active message send
    activeMessageNode();

    // Invoke tutorial 1c: active message serialization
    activeMessageSerialization();

    // Invoke tutorial 1d: active message broadcast
    activeMessageBroadcast();

    // Invoke tutorial 1e: active message group rooted
    activeMessageGroupRoot();

    // Invoke tutorial 1g: callback
    activeMessageCallback();
  }

  /*
   * The barrier (which should not be used in most cases) causes all nodes to
   * wait until they arrive. Barriers can be named (as an optional parameter) so
   * multiple strands can independently make progress. The barrier here is so
   * the output from the above calls are easier to read and not interleaved with
   * the following.
   */
  ::vt::theCollective()->barrier();

  // Invoke tutorial 1f: active message group collective
  activeMessageGroupCollective();

  ::vt::theCollective()->barrier();

  // Invoke tutorial 1h: reduction
  activeMessageReduce();

  // Invoke tutorial 2a: virtual collection creation
  collection();

  // Invoke tutorial 2b: virtual collection reduce
  collectionReduce();

  // Termination with active messages
  activeMessageTerm();


  /*
   * After starting an initialize work (the initial work may only involve node
   * 0) every node will call into the scheduler and spin until termination is
   * reached. The system guarantees that termination will not be detected until
   * all work in the system is finished and will not generate more work (via a
   * incomplete causal chain of events).
   */
  while (!::vt::rt->isTerminated()) {
    ::vt::runScheduler();
  }

  /*
   * All node invoke finalize at the end of the program
   */
  ::vt::CollectiveOps::finalize();

}
