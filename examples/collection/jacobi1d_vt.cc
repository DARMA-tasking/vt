/*
//@HEADER
// *****************************************************************************
//
//                                jacobi1d_vt.cc
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

/// [Jacobi1D example]

//
// This code applies a few steps of the Jacobi iteration to
// the linear system  A x = 0
// where is the tridiagonal matrix with pattern [-1 2 -1]
// The initial guess for x is a made-up non-zero vector.
// The exact solution is the vector 0.
//
// The matrix A is square and invertible.
// The number of rows is ((number of objects) * (number of rows per object))
//
// Such a matrix A is obtained when using 2nd-order finite difference
// for discretizing (-d^2 u /dx^2 = f) on [0, 1] with homogeneous
// Dirichlet condition (u(0) = u(1) = 0) using a uniform grid
// with grid size 1 / ((number of objects) * (number of rows per object) + 1)
//


#include <vt/transport.h>
#include <vt/runnable/invoke.h>

#include <cstdlib>
#include <cassert>
#include <iostream>

static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr double const default_tol = 1.0e-02;

struct NodeObj {
  bool is_finished_ = false;
  struct WorkFinishedMsg : vt::Message {};

  void workFinishedHandler(WorkFinishedMsg*) { is_finished_ = true; }
  bool isWorkFinished() { return is_finished_; }
};
using NodeObjProxy = vt::objgroup::proxy::Proxy<NodeObj>;

struct LinearPb1DJacobi : vt::Collection<LinearPb1DJacobi,vt::Index1D> {

private:

  std::vector<double> tcur_, told_;
  std::vector<double> rhs_;
  size_t iter_ = 0;
  size_t msgReceived_ = 0, totalReceive_ = 0;
  size_t numObjs_ = 1;
  size_t numRowsPerObject_ = 1;
  size_t maxIter_ = 8;
  NodeObjProxy objProxy_;

public:

  explicit LinearPb1DJacobi() :
    tcur_(), told_(), rhs_(), iter_(0),
    msgReceived_(0), totalReceive_(0),
    numObjs_(1), numRowsPerObject_(1), maxIter_(8)
  { }


  using BlankMsg = vt::CollectionMessage<LinearPb1DJacobi>;

  struct LPMsg : vt::CollectionMessage<LinearPb1DJacobi> {

    size_t numObjects = 0;
    size_t nRowPerObject = 0;
    size_t iterMax = 0;
    NodeObjProxy objProxy;

    LPMsg() = default;

    LPMsg(const size_t nobjs, const size_t nrow, const size_t itMax, NodeObjProxy proxy) :
      numObjects(nobjs), nRowPerObject(nrow), iterMax(itMax), objProxy(proxy)
    { }

  };

  struct ReduxMsg : vt::collective::ReduceTMsg<double> {
    ReduxMsg() = default;
    explicit ReduxMsg(double in_val) : ReduceTMsg<double>(in_val) { }
  };

  void checkCompleteCB(ReduxMsg* msg) {
    //
    // Only one object for the reduction will visit
    // this function
    //

    double normRes = msg->getConstVal();

    auto const iter_max_reached = iter_ > maxIter_;
    auto const norm_res_done = normRes < default_tol;

    if (iter_max_reached or norm_res_done) {
      auto const to_print = iter_max_reached ?
        "\n Maximum Number of Iterations Reached. \n\n" :
        fmt::format("\n Max-Norm Residual Reduced by {} \n\n", default_tol);

      fmt::print(to_print);

      // Notify all nodes that computation is finished
      objProxy_.broadcast<NodeObj::WorkFinishedMsg, &NodeObj::workFinishedHandler>();
    } else {
      fmt::print(" ## ITER {} >> Residual Norm = {} \n", iter_, normRes);
    }
  }

  void doIteration() {

    iter_ += 1;

    //
    //--- Copy extremal values
    //
    tcur_[0] = told_[0];
    tcur_[numRowsPerObject_+1] = told_[numRowsPerObject_+1];

    //
    //---- Jacobi iteration step
    //---- A tridiagonal matrix = "tridiag" ( [-1.0  2.0  -1.0] )
    //---- rhs_ right hand side vector
    //
    for (size_t ii = 1; ii <= numRowsPerObject_; ++ii) {
      tcur_[ii] = 0.5*(rhs_[ii] + told_[ii-1] + told_[ii+1]);
    }

    std::copy(tcur_.begin(), tcur_.end(), told_.begin());

    //
    // Compute the maximum entries among the rows on this object
    // We do not take into account the "ghost" entries
    // as they may be "out of date".
    //

    double maxNorm = 0.0;

    for (size_t ii = 1; ii < tcur_.size()-1; ++ii) {
      double val = tcur_[ii];
      maxNorm = (maxNorm > std::fabs(val)) ? maxNorm : std::fabs(val);
    }

    auto proxy = this->getCollectionProxy();
    auto cb = vt::theCB()->makeSend<
      LinearPb1DJacobi,ReduxMsg,&LinearPb1DJacobi::checkCompleteCB
    >(proxy[0]);
    auto msg2 = vt::makeMessage<ReduxMsg>(maxNorm);
    proxy.reduce<vt::collective::MaxOp<double>>(msg2.get(),cb);

  }


  struct VecMsg : vt::CollectionMessage<LinearPb1DJacobi> {
    using MessageParentType = vt::CollectionMessage<LinearPb1DJacobi>;
    vt_msg_serialize_if_needed_by_parent_or_type1(vt::IdxBase);

    VecMsg() = default;

    VecMsg(vt::IdxBase const& in_index, double const& ref) :
      vt::CollectionMessage<LinearPb1DJacobi>(),
      from_index(in_index), val(ref)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | from_index;
      s | val;
    }

    vt::IdxBase from_index = 0;
    double val = 0.0;
  };

  void exchange(VecMsg *msg) {
    // Receive and treat the message from a neighboring object.

    const vt::IdxBase myIdx = getIndex().x();

    if (myIdx > msg->from_index) {
      this->told_[0] = msg->val;
      msgReceived_ += 1;
    }

    if (myIdx < msg->from_index) {
      this->told_[numRowsPerObject_ + 1] = msg->val;
      msgReceived_ += 1;
    }

    // Check whether this 'object' has received all the expected messages.
    if (msgReceived_ == totalReceive_) {
      msgReceived_ = 0;
      doIteration();
    }

  }

  void doIter(BlankMsg *msg) {

    //
    // Treat the particular case of 1 object
    // where no communication is needed.
    // Without this treatment, the code would not iterate.
    //

    if (numObjs_ == 1) {
      doIteration();
      return;
    }
    //---------------------------------------

    //
    // Routine to send information to a different object
    //

    vt::IdxBase const myIdx = getIndex().x();

    //--- Send the values to the left
    auto proxy = this->getCollectionProxy();
    if (myIdx > 0) {
      proxy[myIdx - 1].send<VecMsg, &LinearPb1DJacobi::exchange>(
        myIdx, told_[1]
      );
    }

    //--- Send values to the right
    if (size_t(myIdx) < numObjs_ - 1) {
      proxy[myIdx + 1].send<VecMsg, &LinearPb1DJacobi::exchange>(
        myIdx, told_[numRowsPerObject_]
      );
    }
  }


  void init() {

    tcur_.assign(numRowsPerObject_ + 2, 0.0);
    told_.assign(numRowsPerObject_ + 2, 0.0);
    rhs_.assign(numRowsPerObject_ + 2, 0.0);

    double h = 1.0 / (numRowsPerObject_ * numObjs_ + 1.0);
    int nf = 3 * int(numRowsPerObject_ * numObjs_ + 1) / 4;

    size_t const myIdx = getIndex().x();

    for (size_t ii = 0; ii < tcur_.size(); ++ii) {
      double x0 = ( numRowsPerObject_ * myIdx + ii) * h;
      tcur_[ii] = sin(nf * M_PI * x0 * x0);
    }

    totalReceive_ = 2;

    if (myIdx == 0) {
      tcur_[0] = 0.0;
      totalReceive_ -= 1;
    }

    if (myIdx == numObjs_ - 1) {
      tcur_[numRowsPerObject_+1] = 0.0;
      totalReceive_ -= 1;
    }

    std::copy(tcur_.begin(), tcur_.end(), told_.begin());

  }


  void init(LPMsg* msg) {
    numObjs_ = msg->numObjects;
    numRowsPerObject_ = msg->nRowPerObject;
    maxIter_ = msg->iterMax;
    objProxy_ = msg->objProxy;

    // Initialize the starting vector
    init();
  }

};

bool isWorkDone( vt::objgroup::proxy::Proxy<NodeObj> const& proxy){
  auto const this_node = vt::theContext()->getNode();
  return proxy[this_node].invoke<decltype(&NodeObj::isWorkFinished), &NodeObj::isWorkFinished>();
}

int main(int argc, char** argv) {

  size_t num_objs = default_num_objs;
  size_t numRowsPerObject = default_nrow_object;
  size_t maxIter = 8;

  std::string name(argv[0]);

  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (argc == 1) {
    if (this_node == 0) {
      fmt::print(
        stderr, "{}: using default arguments since none provided\n", name
      );
    }
    num_objs = default_num_objs * num_nodes;
  } else if (argc == 2) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
  }
  else if (argc == 3) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
    numRowsPerObject = static_cast<size_t>(strtol(argv[2], nullptr, 10));
  }
  else if (argc == 4) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
    numRowsPerObject = static_cast<size_t>(strtol(argv[2], nullptr, 10));
    maxIter = static_cast<size_t>(strtol(argv[3], nullptr, 10));
  }
  else {
    fmt::print(
      stderr, "usage: {} <num-objects> <num-rows-per-object> <maxiter>\n",
      name
    );
    return 1;
  }

  // Object group of all nodes that take part in computation
  // Used to determine whether the computation is finished
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>();

  // Create the decomposition into objects
  using BaseIndexType = typename vt::Index1D::DenseIndexType;
  auto range = vt::Index1D(static_cast<BaseIndexType>(num_objs));

  auto col_proxy =
    vt::theCollection()->constructCollective<LinearPb1DJacobi>(range);

  vt::runInEpochCollective([col_proxy, grp_proxy, num_objs, numRowsPerObject, maxIter]{
    col_proxy.broadcastCollective<LinearPb1DJacobi::LPMsg, &LinearPb1DJacobi::init>(
      num_objs, numRowsPerObject, maxIter, grp_proxy
    );
  });

  while(!isWorkDone(grp_proxy)){
    vt::runInEpochCollective([col_proxy]{
      col_proxy.broadcastCollective<
        LinearPb1DJacobi::BlankMsg, &LinearPb1DJacobi::doIter
      >();
    });

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;
}

/// [Jacobi1D example]
