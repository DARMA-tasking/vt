/*
//@HEADER
// ************************************************************************
//
//                          jacobi1d_vt.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/


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


#include "vt/transport.h"

#include <cstdlib>
#include <cassert>
#include <iostream>


using namespace ::vt;


static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr double const default_tol = 1.0e-02;


struct LinearPb1DJacobi : vt::Collection<LinearPb1DJacobi,Index1D> {

private:

  std::vector<double> tcur_, told_;
  std::vector<double> rhs_;
  size_t iter_ = 0;
  size_t msgReceived_ = 0, totalReceive_ = 0;
  size_t numObjs_ = 1;
  size_t numRowsPerObject_ = 1;
  size_t maxIter_ = 8;

public:

  explicit LinearPb1DJacobi() :
    tcur_(), told_(), rhs_(), iter_(0),
    msgReceived_(0), totalReceive_(0),
    numObjs_(1), numRowsPerObject_(1), maxIter_(8)
  { }


  using BlankMsg = vt::CollectionMessage<LinearPb1DJacobi>;

  struct InitMsg : vt::collective::ReduceNoneMsg { };

  struct LPMsg : vt::CollectionMessage<LinearPb1DJacobi> {

    size_t numObjects = 0;
    size_t nRowPerObject = 0;
    size_t iterMax = 0;

    LPMsg() = default;

    LPMsg(const size_t nobjs, const size_t nrow, const size_t itMax) :
      CollectionMessage<LinearPb1DJacobi>(),
      numObjects(nobjs), nRowPerObject(nrow), iterMax(itMax)
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
    auto const iter = iter_;
    auto const maxIter = maxIter_;

    if ((iter <= maxIter) and (normRes >= default_tol)) {
      fmt::print(" ## ITER {} >> Residual Norm = {} \n", iter, normRes);
      //
      // Start a new iteration
      //
      auto proxy = getCollectionProxy();
      auto loopMsg = makeSharedMessage<BlankMsg>();
      proxy.broadcast<BlankMsg, &LinearPb1DJacobi::sendInfo>(loopMsg);
    }
    else if (iter > maxIter) {
      fmt::print("\n Maximum Number of Iterations Reached. \n\n");
    }
    else{
      fmt::print("\n Max-Norm Residual Reduced by {} \n\n", default_tol);
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
    auto cb = theCB()->makeSend<
      LinearPb1DJacobi,ReduxMsg,&LinearPb1DJacobi::checkCompleteCB
    >(proxy[0]);
    auto msg2 = makeMessage<ReduxMsg>(maxNorm);
    proxy.reduce<collective::MaxOp<double>>(msg2.get(),cb);

  }


  struct VecMsg : vt::CollectionMessage<LinearPb1DJacobi> {

    IdxBase from_index = 0;
    double val = 0.0;

    VecMsg() = default;

    VecMsg(IdxBase const& in_index, double const& ref) :
      vt::CollectionMessage<LinearPb1DJacobi>(),
      from_index(in_index), val(ref)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      s | from_index;
      s | val;
    }

  };


  void exchange(VecMsg *msg) {

    // Receive and treat the message from a neighboring object.

    const IdxBase myIdx = getIndex().x();

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

  void doneInit(InitMsg *msg) {
    sendInfo(nullptr);
  }

  void sendInfo(BlankMsg *msg) {

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
      auto leftMsg = vt::makeSharedMessage<VecMsg>(myIdx, told_[1]);
      proxy[myIdx-1].send<VecMsg, &LinearPb1DJacobi::exchange>(leftMsg);
    }

    //--- Send values to the right
    if (size_t(myIdx) < numObjs_ - 1) {
      auto rightMsg = vt::makeSharedMessage<VecMsg>(
        myIdx, told_[numRowsPerObject_]
      );
      proxy[myIdx+1].send<VecMsg, &LinearPb1DJacobi::exchange>(rightMsg);
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


  void solve(LPMsg* msg) {

    numObjs_ = msg->numObjects;
    numRowsPerObject_ = msg->nRowPerObject;
    maxIter_ = msg->iterMax;

    // Initialize the starting vector
    init();

    // Wait for all initializations to complete, reduce, and then:
    // Start the algorithm with a neighbor-to-neighbor communication
    using CollType = LinearPb1DJacobi;
    auto proxy = this->getCollectionProxy();
    auto cb = theCB()->makeBcast<CollType, InitMsg, &CollType::doneInit>(proxy);
    auto empty = makeMessage<InitMsg>();
    proxy.reduce(empty.get(),cb);
  }

};


int main(int argc, char** argv) {

  size_t num_objs = default_num_objs;
  size_t numRowsPerObject = default_nrow_object;
  size_t maxIter = 8;

  std::string name(argv[0]);

  vt::CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

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

  if (this_node == 0) {

    // Create the decomposition into objects
    using BaseIndexType = typename Index1D::DenseIndexType;
    auto const& range = Index1D(static_cast<BaseIndexType>(num_objs));

    auto proxy = vt::theCollection()->construct<LinearPb1DJacobi>(range);
    auto rootMsg = makeSharedMessage<LinearPb1DJacobi::LPMsg>(
      num_objs, numRowsPerObject, maxIter
    );
    proxy.broadcast<LinearPb1DJacobi::LPMsg,&LinearPb1DJacobi::solve>(rootMsg);

  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;

}

