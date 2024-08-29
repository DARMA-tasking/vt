/*
//@HEADER
// *****************************************************************************
//
//                                do_flops_perf.cc
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
#include <vt/metrics/perf_data.h>

#include <cstdlib>
#include <cassert>
#include <iostream>

/// [Do Flops example]

#include <vt/transport.h>
#include <vt/runnable/invoke.h>

#include <cstdlib>
#include <cassert>
#include <iostream>

static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 100;
static constexpr double const default_tol = 1.0e-02;
static constexpr std::size_t const default_flops_per_iter = 100000;

volatile double a = 0.5, b = 2.2;

void
dummy( void *array )
{
/* Confuse the compiler so as not to optimize
   away the flops in the calling routine    */
/* Cast the array as a void to eliminate unused argument warning */
	( void ) array;
}

void
do_flops( int n )
{
	int i;
	double c = 0.11;

	for ( i = 0; i < n; i++ ) {
		c += a * b;
	}
	dummy( ( void * ) &c );
}

double pi(uint64_t n) {
  double sum = 0.0;
  int sign = 1;
  for (uint64_t i = 0; i < n; ++i) {
    sum += sign/(2.0*i+1.0);
    sign *= -1;
  }
  return 4.0*sum;
}

struct NodeObj {
  bool is_finished_ = false;
  void workFinishedHandler() { is_finished_ = true; }
  bool isWorkFinished() { return is_finished_; }
};
using NodeObjProxy = vt::objgroup::proxy::Proxy<NodeObj>;

struct GenericWork : vt::Collection<GenericWork, vt::Index1D> {

private:
  size_t iter_ = 0;
  size_t msgReceived_ = 0, totalReceive_ = 0;
  size_t numObjs_ = 1;
  size_t flopsPerIter_ = default_flops_per_iter;
  size_t maxIter_ = 8;
  NodeObjProxy objProxy_;

public:
  explicit GenericWork() :
    iter_(0), msgReceived_(0), totalReceive_(0),
    numObjs_(1), flopsPerIter_(default_flops_per_iter), maxIter_(8)
  { }

  using BlankMsg = vt::CollectionMessage<GenericWork>;

  struct WorkMsg : vt::CollectionMessage<GenericWork> {
    size_t numObjects = 0;
    size_t flopsPerIter = 0;
    size_t iterMax = 0;
    NodeObjProxy objProxy;

    WorkMsg() = default;

    WorkMsg(const size_t nobjs, const size_t flops, const size_t itMax, NodeObjProxy proxy) :
      numObjects(nobjs), flopsPerIter(flops), iterMax(itMax), objProxy(proxy)
    { }
  };

  void checkCompleteCB() {
    auto const iter_max_reached = iter_ > maxIter_;

    if (iter_max_reached) {
      fmt::print("\n Maximum Number of Iterations Reached. \n\n");
      objProxy_.broadcast<&NodeObj::workFinishedHandler>();
    } else {
      fmt::print(" ## ITER {} completed. \n", iter_);
    }
  }

  void doIteration() {
    iter_ += 1;
    fmt::print("-- Starting Iteration --\n");

    vt::thePerfData()->startTaskMeasurement(vt::theContext()->getTask());
    
    // ----------------------------------------------------------
    // test non packed double precision floating point operations
    // should result in ~4*n of these operations

    double p;
    p = pi(10000000);
    fmt::print("pi: {}\n", p);
    // ----------------------------------------------------------

    auto proxy = this->getCollectionProxy();
    proxy.reduce<&GenericWork::checkCompleteCB, vt::collective::MaxOp>(
      proxy[0]
    );

    vt::thePerfData()->stopTaskMeasurement(vt::theContext()->getTask());
    std::unordered_map<std::string, uint64_t> res = vt::thePerfData()->getTaskMeasurements(vt::theContext()->getTask());
    for (auto [name, value] : res) {
      fmt::print("  {}: {}\n", name, value);
    }

    fmt::print("-- Stopping Iteration --\n");
  }

  struct VecMsg : vt::CollectionMessage<GenericWork> {
    using MessageParentType = vt::CollectionMessage<GenericWork>;
    vt_msg_serialize_if_needed_by_parent_or_type1(vt::IdxBase);

    VecMsg() = default;

    VecMsg(vt::IdxBase const& in_index) :
      vt::CollectionMessage<GenericWork>(),
      from_index(in_index)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | from_index;
    }

    vt::IdxBase from_index = 0;
  };

  void exchange(VecMsg *msg) {
    msgReceived_ += 1;

    if (msgReceived_ == totalReceive_) {
      msgReceived_ = 0;
      doIteration();
    }
  }

  void doIter([[maybe_unused]] BlankMsg *msg) {
    if (numObjs_ == 1) {
      doIteration();
      return;
    }

    vt::IdxBase const myIdx = getIndex().x();
    auto proxy = this->getCollectionProxy();


    if (myIdx > 0) {
      proxy[myIdx - 1].send<VecMsg, &GenericWork::exchange>(
        myIdx
      );
    }

    if (size_t(myIdx) < numObjs_ - 1) {
      proxy[myIdx + 1].send<VecMsg, &GenericWork::exchange>(
        myIdx
      );
    }
  }

  void init() {
    totalReceive_ = 2;

    if (getIndex().x() == 0) {
      totalReceive_ -= 1;
    }

    if (getIndex().x() == numObjs_ - 1) {
      totalReceive_ -= 1;
    }
  }

  void init(WorkMsg* msg) {
    numObjs_ = msg->numObjects;
    flopsPerIter_ = msg->flopsPerIter;
    maxIter_ = msg->iterMax;
    objProxy_ = msg->objProxy;

    init();
  }
};

bool isWorkDone(vt::objgroup::proxy::Proxy<NodeObj> const& proxy) {
  auto const this_node = vt::theContext()->getNode();
  return proxy[this_node].invoke<&NodeObj::isWorkFinished>();
}

int main(int argc, char** argv) {
  size_t num_objs = default_num_objs;
  size_t flopsPerIter = default_flops_per_iter;
  size_t maxIter = 8;

  std::string name(argv[0]);

  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (argc == 1) {
    if (this_node == 0) {
      fmt::print(stderr, "{}: using default arguments since none provided\n", name);
    }
    num_objs = default_num_objs * num_nodes;
  } else if (argc == 2) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
  } else if (argc == 3) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
    flopsPerIter = static_cast<size_t>(strtol(argv[2], nullptr, 10));
  } else if (argc == 4) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
    flopsPerIter = static_cast<size_t>(strtol(argv[2], nullptr, 10));
    maxIter = static_cast<size_t>(strtol(argv[3], nullptr, 10));
  } else {
    fmt::print(stderr, "usage: {} <num-objects> <flops-per-iter> <maxiter>\n", name);
    return 1;
  }

  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>("examples_generic_work");
  using BaseIndexType = typename vt::Index1D::DenseIndexType;
  auto range = vt::Index1D(static_cast<BaseIndexType>(num_objs));

  auto col_proxy = vt::makeCollection<GenericWork>("examples_generic_work")
    .bounds(range)
    .bulkInsert()
    .wait();

  vt::runInEpochCollective([col_proxy, grp_proxy, num_objs, flopsPerIter, maxIter]{
    col_proxy.broadcastCollective<GenericWork::WorkMsg, &GenericWork::init>(
      num_objs, flopsPerIter, maxIter, grp_proxy
    );
  });

  while(!isWorkDone(grp_proxy)) {
    vt::runInEpochCollective([col_proxy]{
      col_proxy.broadcastCollective<
        GenericWork::BlankMsg, &GenericWork::doIter
      >();
    });

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;
}
/// [Do Flops example]
