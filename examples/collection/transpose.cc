/*
//@HEADER
// *****************************************************************************
//
//                                 transpose.cc
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

#include <vt/transport.h>

#include <vector>

static constexpr int const num_pieces = 4;
static constexpr int const num_elem = 1024;
static constexpr int const block_size = num_elem / num_pieces;

template <typename ColT>
struct SolveMsg : vt::CollectionMessage<ColT> {};

template <typename ColT>
struct RequestDataMsg : vt::CollectionMessage<ColT> {
  explicit RequestDataMsg(vt::NodeType in_node) : node_(in_node) { }
  vt::NodeType node_;
};

struct InitMsg : vt::collective::ReduceNoneMsg { };

struct DataMsg : vt::Message {
  using MessageParentType = vt::Message;
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

struct SubSolveMsg : vt::Message {
  explicit SubSolveMsg(vt::VirtualProxyType in_proxy)
    : coll_proxy_(in_proxy)
  { }

  vt::VirtualProxyType coll_proxy_ = vt::no_vrt_proxy;
};

struct ProxyMsg : vt::Message {
  explicit ProxyMsg(vt::VirtualProxyType in_proxy)
    : proxy_(in_proxy)
  { }

  vt::VirtualProxyType proxy_ = vt::no_vrt_proxy;
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

  MPI_Comm sub_comm_ = MPI_COMM_WORLD;
  int32_t sub_rank_ = -1;
  int32_t sub_size_ = -1;
  int32_t needed_blocks_ = 0;
  int32_t have_blocks_ = 0;
  std::vector<double> solver_data_;
};

// Static instance of the solver info (one-per-node)
/*static*/ SubSolveInfo solver_info = {};

// The VT collection with blocks of data
struct Block : vt::Collection<Block, vt::Index1D> {

  Block() {
    fmt::print(
      "construct: node={}\n",
      vt::theContext()->getNode()
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
    vt::Collection<Block, vt::Index1D>::serialize(s);
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
    auto data_msg = vt::makeMessage<DataMsg>(data_,from_idx);
    vt::theMsg()->sendMsg<SubSolveInfo::solveDataIncoming>(
      requesting_node, data_msg
    );
  }

  void doneInit(InitMsg* msg) {
    if (getIndex().x() == 0) {
      auto proxy = this->getCollectionProxy();
      auto proxy_msg = vt::makeMessage<ProxyMsg>(proxy.getProxy());
      vt::theMsg()->broadcastMsg<SetupGroup>(proxy_msg);
    }
  }

  void solve(SolveMsg<Block>* msg) {
    // Invoke initialize here so that the index is ready
    initialize();
    // Wait for all initializations to complete
    auto proxy = this->getCollectionProxy();
    auto cb = vt::theCB()->makeBcast<Block, InitMsg, &Block::doneInit>(proxy);
    auto empty = vt::makeMessage<InitMsg>();
    proxy.reduce(empty.get(), cb);
  }

private:
  std::vector<double> data_ = {};
};

//using ActiveMapTypedFnType = NodeType(IndexT*, IndexT*, NodeType);
template <typename IndexT>
vt::NodeType my_map(IndexT* idx, IndexT* max_idx, vt::NodeType num_nodes) {
  // simple round-robin for 1-d only.
  return idx->x() % num_nodes;
}

// group-targeted handler for the sub-solve
/*static*/ void SubSolveInfo::subSolveHandler(SubSolveMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  auto const group_id = vt::envelopeGetGroup(msg->env);
  MPI_Comm sub_comm = vt::theGroup()->getGroupComm(group_id);
  int sub_size = 0, sub_rank = 0;
  MPI_Comm_size(sub_comm, &sub_size);
  MPI_Comm_rank(sub_comm, &sub_rank);

  fmt::print(
    "subSolveHandler: node={}, num={}, sub_rank={}, sub_size={}\n",
    this_node, num_nodes, sub_rank, sub_size
  );

  vtAssert(this_node % 2 == 0, "Must be even");
  vtAssert(sub_size == num_nodes / 2, "Must be half for all nodes");
  vtAssert(sub_rank == this_node / 2, "Correct rank compared to global");

  // Reconstruct the collection proxy type. Note: you can transfer the typed
  // proxy and skip this step, but then the proxy has to carry the type or be
  // templated
  vt::CollectionProxy<Block, vt::Index1D> proxy(msg->coll_proxy_);

  // Do a simple "blocked" map from collection pieces to node
  auto const blocks_to_node = num_pieces / sub_size;
  auto const solver_local_elms = blocks_to_node * block_size;
  solver_info.sub_comm_ = sub_comm;
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
      // Here we will send "this_node" to indicate which nod it should come back
      // to.  Eventually, I will implement a "sub_rank" in VT which can use the
      // sub-rank instead of the global node id.
      proxy[block_id].send<RequestDataMsg<Block>,&Block::dataRequest>(this_node);
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

static void solveGroupSetup(vt::NodeType this_node, vt::VirtualProxyType coll_proxy) {
  auto const& is_even_node = this_node % 2 == 0;

  // This is how you would explicitly create/get a new communicator for this
  // group. Because of the change I made, there is automatically one created for
  // you, so unless you want to restructure the communicator (reorganize the
  // ranks), you can use the one that is created automatically by VT. See below:
  // theGroup()->getGroupComm(...)

  // auto const& the_comm = vt::theContext()->getComm();
  // MPI_Comm manual_sub_comm;
  // MPI_Comm_split(the_comm, is_even_node, this_node, &manual_sub_comm);
  // solver_info.sub_comm_ = manual_sub_comm;

  fmt::print(
    "solveGroupSetup: node={}, is_even_node={}\n",
    this_node, is_even_node
  );

  vt::theGroup()->newGroupCollective(
    is_even_node, [=](vt::GroupType group_id){
      fmt::print("{}: Group is created: id={:x}\n", this_node, group_id);
      if (this_node == 1) {
        auto msg = vt::makeMessage<SubSolveMsg>(coll_proxy);
        vt::envelopeSetGroup(msg->env, group_id);
        vt::theMsg()->broadcastMsg<SubSolveInfo::subSolveHandler>(msg);
      }
    }, true
  );

}

void SetupGroup::operator()(ProxyMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();
  fmt::print("SetupGroup: node={}\n", this_node);
  // Example using the group collective
  solveGroupSetup(this_node, msg->proxy_);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  auto range = vt::Index1D(num_pieces);
  auto proxy = vt::makeCollection<Block>("examples_transpose")
    .bounds(range)
    .bulkInsert()
    .mapperFunc<my_map>()
    .wait();

  if (this_node == 0) {
    proxy.broadcast<SolveMsg<Block>, &Block::solve>();
  }

  vt::finalize();
  return 0;
}
