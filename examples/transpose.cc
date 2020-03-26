/*
//@HEADER
// *****************************************************************************
//
//                                 transpose.cc
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

using namespace vt;

static constexpr int const num_pieces = 4;
static constexpr int const num_elem = 1024;
static constexpr int const block_size = num_elem / num_pieces;

template <typename ColT>
struct SolveMsg : CollectionMessage<ColT> {};

template <typename ColT>
struct RequestDataMsg : CollectionMessage<ColT> {
  RequestDataMsg() = default;
  RequestDataMsg(NodeType in_node) : node_(in_node) { }
  NodeType node_;
};

struct InitMsg : ::vt::collective::ReduceNoneMsg { };

struct DataMsg : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_required(); // by payload_

  DataMsg() = default;

  DataMsg(std::vector<double> const& payload_in, int from_idx_in)
    : payload_(payload_in), from_idx_(from_idx_in)
  {}

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | payload_;
    s | from_idx_;
  }

  std::vector<double> payload_;
  int from_idx_ = 0;
};

struct SubSolveMsg : ::vt::Message {
  SubSolveMsg() = default;
  SubSolveMsg(VirtualProxyType in_proxy)
    : coll_proxy_(in_proxy)
  { }

  VirtualProxyType coll_proxy_ = no_vrt_proxy;
};

struct ProxyMsg : vt::Message {
  ProxyMsg() = default;
  ProxyMsg(VirtualProxyType in_proxy) : proxy_(in_proxy) { }
  VirtualProxyType proxy_ = no_vrt_proxy;
};

struct SetupGroup {
  void operator()(ProxyMsg* msg);
};

// Node-local (MPI rank local) info used for the group
struct SubSolveInfo {
  SubSolveInfo() = default;

  void runKernel() {
    // Implement the real kernel here: using the sub communicator
    auto const& block_offset = sub_rank_ * needed_blocks_;
    // This will just check that the transpose took place properly
    for (auto i = 0; i < have_blocks_ * block_size; i++) {
      vtAssert(
        solver_data_[i] == block_offset * block_size + i, "Correct value"
      );
    }
  }

  // Handlers for getting and setting up data
  static void solveDataIncoming(DataMsg* msg);
  static void subSolveHandler(SubSolveMsg* msg);

  MPI_Comm sub_comm = MPI_COMM_WORLD;
  int32_t sub_rank_ = -1;
  int32_t sub_size_ = -1;
  int32_t needed_blocks_ = 0;
  int32_t have_blocks_ = 0;
  std::vector<double> solver_data_;
};

// Static instance of the solver info (one-per-node)
/*static*/ SubSolveInfo solver_info = {};

// The VT collection with blocks of data
struct Block : Collection<Block,Index1D> {

  Block() = default;
  Block(int num_elm, int n_pieces) {
    ::fmt::print(
      "construct: node={}, elm={}, pieces={}\n",
      theContext()->getNode(), num_elm, n_pieces
    );
  }

  void initialize() {
    auto idx = this->getIndex();
    data_.resize(block_size);
    for (auto i = 0; i < block_size; i++) {
      data_[i] = idx.x() * block_size + i;
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Collection<Block,Index1D>::serialize(s);
    s | data_;
  }

  std::vector<double> const& getData() { return data_; }

  void dataRequest(RequestDataMsg<Block>* msg) {
    auto const& requesting_node = msg->node_;
    ::fmt::print(
      "dataRequest: idx={}: requesting_node={}\n",
      getIndex(), requesting_node
    );
    auto const from_idx = getIndex().x();
    auto data_msg = makeSharedMessage<DataMsg>(data_,from_idx);
    theMsg()->sendMsg<DataMsg,SubSolveInfo::solveDataIncoming>(
      requesting_node, data_msg
    );
  }

  void doneInit(InitMsg *msg) {
    if (theContext()->getNode() == 0) {
      auto proxy = this->getCollectionProxy();
      auto proxy_msg = ::vt::makeSharedMessage<ProxyMsg>(proxy.getProxy());
      theMsg()->broadcastMsg<SetupGroup>(proxy_msg);
      // Invoke it locally: broadcast sends to all other nodes
      auto proxy_msg_local = ::vt::makeSharedMessage<ProxyMsg>(proxy.getProxy());
      SetupGroup()(proxy_msg_local);
    }
  }

  void solve(SolveMsg<Block>* msg) {
    // Invoke initialize here so that the index is ready
    initialize();
    // Wait for all initializations to complete
    auto proxy = this->getCollectionProxy();
    auto cb = theCB()->makeBcast<Block, InitMsg, &Block::doneInit>(proxy);
    auto empty = makeMessage<InitMsg>();
    proxy.reduce(empty.get(), cb);
  }

private:
  std::vector<double> data_ = {};
};

//using ActiveMapTypedFnType = NodeType(IndexT*, IndexT*, NodeType);
template <typename IndexT>
::vt::NodeType my_map(IndexT* idx, IndexT* max_idx, NodeType num_nodes) {
  // simple round-robin for 1-d only.
  return idx->x() % num_nodes;
}

// group-targeted handler for the sub-solve
/*static*/ void SubSolveInfo::subSolveHandler(SubSolveMsg* msg) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  auto const group_id = envelopeGetGroup(msg->env);
  MPI_Comm sub_comm = theGroup()->getGroupComm(group_id);
  int sub_size = 0, sub_rank = 0;
  MPI_Comm_size(sub_comm, &sub_size);
  MPI_Comm_rank(sub_comm, &sub_rank);

  ::fmt::print(
    "subSolveHandler: node={}, num={}, sub_rank={}, sub_size={}\n",
    this_node, num_nodes, sub_rank, sub_size
  );

  vtAssert(this_node % 2 == 0, "Must be even");
  vtAssert(sub_size == num_nodes / 2, "Must be half for all nodes");
  vtAssert(sub_rank == this_node / 2, "Correct rank compared to global");

  // Reconstruct the collection proxy type. Note: you can transfer the typed
  // proxy and skip this step, but then the proxy has to carry the type or be
  // templated
  CollectionProxy<Block,Index1D> proxy(msg->coll_proxy_);

  // Do a simple "blocked" map from collection pieces to node
  auto const blocks_to_node = num_pieces / sub_size;
  auto const solver_local_elms = blocks_to_node * block_size;
  solver_info.sub_comm = sub_comm;
  solver_info.sub_rank_ = sub_rank;
  solver_info.sub_size_ = sub_size;
  solver_info.needed_blocks_ = blocks_to_node;
  solver_info.have_blocks_ = 0;
  solver_info.solver_data_.resize(solver_local_elms);

  for (auto i = 0; i < blocks_to_node; i++) {
    auto const block_offset = sub_rank * blocks_to_node;
    auto const block_id = block_offset + i;
    auto const data_idx_offset = i * block_size;
    auto elm_ptr = proxy[block_id].tryGetLocalPtr();
    ::fmt::print("block_id={}, elm_ptr={}\n", block_id, print_ptr(elm_ptr));

    if (elm_ptr) {
      // It's local!
      auto const& data = elm_ptr->getData();

      // Performing a local copy here, but it could be kept as a reference or
      // pointer to the collection element if the data exists in the collection
      // element or it could be pulled from Trilinos
      for (auto j = 0; j < block_size; j++) {
        solver_info.solver_data_[data_idx_offset + j] = data[j];
      }

      solver_info.have_blocks_++;
    } else {
      // It's a remote collection block
      auto msg2 = makeSharedMessage<RequestDataMsg<Block>>(this_node);
      // Here we will send "this_node" to indicate which nod it should come back
      // to.  Eventually, I will implement a "sub_rank" in VT which can use the
      // sub-rank instead of the global node id.
      proxy[block_id].send<RequestDataMsg<Block>,&Block::dataRequest>(msg2);
    }
  }

  if (solver_info.needed_blocks_ == solver_info.have_blocks_) {
    solver_info.runKernel();
  }
}

/*static*/ void SubSolveInfo::solveDataIncoming(DataMsg* msg) {
  auto const global_offset = solver_info.needed_blocks_ * solver_info.sub_rank_;
  auto const block_offset = msg->from_idx_ - (global_offset);
  auto const data_idx_offset = block_offset * block_size;

  ::fmt::print(
    "solveDataIncoming: from_idx={}, offset={}, data_idx={}\n", msg->from_idx_,
    block_offset, data_idx_offset
  );

  for (auto j = 0; j < block_size; j++) {
    solver_info.solver_data_[data_idx_offset + j] = msg->payload_[j];
  }

  solver_info.have_blocks_++;

  if (solver_info.needed_blocks_ == solver_info.have_blocks_) {
    solver_info.runKernel();
  }
}

static void solveGroupSetup(NodeType this_node, VirtualProxyType coll_proxy) {
  auto const& is_even_node = this_node % 2 == 0;

  auto const& the_comm = theContext()->getComm();
  (void)the_comm;  // don't warn about unused variable

  // This is how you would explicitly create/get a new communicator for this
  // group. Because of the change I made, there is automatically one created for
  // you, so unless you want to restructure the communicator (reorganize the
  // ranks), you can use the one that is created automatically by VT. See below:
  // theGroup()->getGroupComm(...)

  // MPI_Comm manual_sub_comm;
  // MPI_Comm_split(the_comm, is_even_node, this_node, &manual_sub_comm);
  // solver_info.sub_comm = manual_sub_comm;

  ::fmt::print(
    "solveGroupSetup: node={}, is_even_node={}\n",
    this_node, is_even_node
  );

  theGroup()->newGroupCollective(
    is_even_node, [=](GroupType group_id){
      fmt::print("Group is created: id={:x}\n", group_id);
      if (this_node == 1) {
        auto msg = makeSharedMessage<SubSolveMsg>(coll_proxy);
        envelopeSetGroup(msg->env, group_id);
        theMsg()->broadcastMsg<SubSolveMsg,SubSolveInfo::subSolveHandler>(msg);
      }
    }, true
  );

}

void SetupGroup::operator()(ProxyMsg* msg) {
  auto const& this_node = theContext()->getNode();
  ::fmt::print("SetupGroup: node={}\n", this_node);
  // Example using the group collective
  solveGroupSetup(this_node, msg->proxy_);
}

int main(int argc, char** argv) {
  ::vt::CollectiveOps::initialize(argc,argv);

  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    auto const& range = Index1D(num_pieces);
    auto proxy = theCollection()->construct<Block,my_map>(
      range,num_elem,num_pieces
    );
    //
    auto msg = ::vt::makeSharedMessage<SolveMsg<Block>>();
    proxy.broadcast<SolveMsg<Block>, &Block::solve>(msg);
  }

  while (!::vt::rt->isTerminated()) {
    runScheduler();
  }

  ::vt::CollectiveOps::finalize();
  return 0;
}
