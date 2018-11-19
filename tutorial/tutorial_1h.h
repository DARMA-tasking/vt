
#include "vt/transport.h"

namespace vt { namespace tutorial {

//                       Reduce Message VT Base Class
//                 \--------------------------------------------/
//                  \                                          /
//                   \                            Reduce Data /
//                    \                          \-----------/
//                     \                          \         /
struct ReduceDataMsg : ::vt::collective::ReduceTMsg<int32_t> {};


// Functor that is the target of the reduction
struct ReduceResult {
  void operator()(ReduceDataMsg* msg) {
    NodeType const num_nodes = ::vt::theContext()->getNumNodes();
    fmt::print("reduction value={}\n", msg->getConstVal());
    assert(num_nodes * 50 == msg->getConstVal());
  }
};


// Tutorial code to demonstrate using a callback
static inline void activeMessageReduce() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * Perform reduction over all the nodes.
   */

  // This is the type of the reduction (uses the plus operator over the data
  // type). Once can implement their own data type and overload the plus
  // operator for the combine during the reduce
  using ReduceOp = ::vt::collective::PlusOp<int32_t>;

  NodeType const root_reduce_node = 0;

  auto reduce_msg = ::vt::makeSharedMessage<ReduceDataMsg>();

  // Get a reference to the value to set it in this reduce msg
  reduce_msg->getVal() = 50;

  ::vt::theCollective()->reduce<
    ReduceDataMsg,
    ReduceDataMsg::msgHandler<ReduceDataMsg, ReduceOp, ReduceResult>
  >(root_reduce_node, reduce_msg);

}

}} /* end namespace vt::tutorial */
