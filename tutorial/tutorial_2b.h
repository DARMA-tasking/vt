
#include "vt/transport.h"

namespace vt { namespace tutorial {

// Forward declaration for message
struct ColRedMsg;

//                  VT Base Class for a collection
//            \------------------------------------------/
//             \                                        /
//              \                              Index   /
//               \                          \---------/
//                \                          \       /
struct ReduceCol : ::vt::Collection<ReduceCol,Index1D> {

  void reduceHandler(ColRedMsg* msg);

};

//                  VT Base Message for Collections
//               \-----------------------------------/
//                \                                 /
struct ColRedMsg : ::vt::CollectionMessage<ReduceCol> { };

//                    Reduce Message VT Base Class
//              \-------------------------------------------/
//               \                                         /
//                \                           Reduce Type /
//                 \                          \----------/
//                  \                          \        /
struct ReduceMsg : ::vt::collective::ReduceTMsg<int32_t> {};

// Functor that is the target of the collection reduction
struct PrintReduceResult {
  void operator()(ReduceMsg* msg) {
    fmt::print("val={}\n", msg->getConstVal());
  }
};


void ReduceCol::reduceHandler(ColRedMsg* msg) {
  auto cur_node = theContext()->getNode();
  auto idx = this->getIndex();

  //::fmt::print("MyCol::reduceHandler index={}, node={}\n", idx.x(), cur_node);

  using ReduceOp = vt::collective::PlusOp<int32_t>;

  auto proxy = getCollectionProxy();
  auto reduce_msg = makeSharedMessage<ReduceMsg>();

  // Get a reference to the value to set it in this reduce msg
  reduce_msg->getVal() = 100;

  // Invoke the reduce!
  proxy.reduce<
    ReduceMsg,
    ReduceMsg::msgHandler<ReduceMsg, ReduceOp, PrintReduceResult>
  >(reduce_msg);
}

// Tutorial code to demonstrate reducing a collection
static inline void collectionReduce() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * This is an example of reducing over a virtual context collection
   */

  if (this_node == 0) {
    // Range of 32 elements for the collection
    auto range = ::vt::Index1D(32);
    // Construct the collection: invoked by one node. By default, the elements
    // will be block mapped to the nodes
    auto proxy = theCollection()->construct<ReduceCol>(range);

    // Broadcast a message to the entire collection. The reduceHandler will be
    // invoked on every element to the collection
    auto msg = ::vt::makeSharedMessage<ColRedMsg>();
    proxy.broadcast<ColRedMsg,&ReduceCol::reduceHandler>(msg);
  }
}

}} /* end namespace vt::tutorial */
