/*
//@HEADER
// *****************************************************************************
//
//                                jacobi1d_vt.cc
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
#include <vt/messaging/collection_chain_set.h>

#include <cstdlib>
#include <cassert>
#include <iostream>

static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr std::size_t const check_conv_freq = 1;
static constexpr double const default_tol = 1.0e-02;

struct NodeObj;

using NodeObjProxy = vt::objgroup::proxy::Proxy<NodeObj>;

struct LinearPb1DJacobi : vt::Collection<LinearPb1DJacobi,vt::Index1D> {
private:
  std::vector<double> tcur_, told_;
  std::vector<double> rhs_;
  size_t iter_ = 0;
  size_t numObjs_ = 1;
  size_t numRowsPerObject_ = 1;
  size_t maxIter_ = 8;
  NodeObjProxy objProxy_;

public:

  explicit LinearPb1DJacobi() :
    tcur_(), told_(), rhs_(), iter_(0),
    numObjs_(1), numRowsPerObject_(1), maxIter_(8)
  { }

  void kernel() {
    vt_print(gen, "kernel {}\n", this->getIndex());
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
  }

  double computeMaxNorm() {
    // Compute the maximum entries among the rows on this object We do not take
    // into account the "ghost" entries as they may be "out of date".
    double maxNorm = 0.0;
    for (size_t ii = 1; ii < tcur_.size()-1; ++ii) {
      double val = tcur_[ii];
      maxNorm = (maxNorm > std::fabs(val)) ? maxNorm : std::fabs(val);
    }
    return maxNorm;
  }

  void reduceMaxNorm(vt::Callback<double> cb) {
    vt_print(gen, "reduceMaxNorm {}\n", this->getIndex());

    auto proxy = this->getCollectionProxy();
    proxy.reduce<vt::collective::MaxOp>(cb, computeMaxNorm());
  }

  void sendLeft() {
    // Send the values to the left
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    vt_print(gen, "sendLeft {}\n", idx);
    if (idx.x() > 0) {
      proxy[idx.x() - 1].send<&LinearPb1DJacobi::exchange>(idx.x(), told_[1]);
    }
  }

  void sendRight() {
    // Send the values to the right
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    vt_print(gen, "sendRight {}\n", idx);
    if (size_t(idx.x()) < numObjs_ - 1) {
      proxy[idx.x() + 1].send<&LinearPb1DJacobi::exchange>(idx.x(), told_[numRowsPerObject_]);
    }
  }

  void exchange(vt::IdxBase from_index, double val) {
    // Receive and treat the message from a neighboring object.
    auto idx = this->getIndex();
    vt_print(gen, "exchange {}, from={}\n", idx, from_index);
    if (idx.x() > from_index) {
      told_[0] = val;
    }
    if (idx.x() < from_index) {
      told_[numRowsPerObject_ + 1] = val;
    }
  }

  void initializeData() {
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

    if (myIdx == 0) {
      tcur_[0] = 0.0;
    }

    if (myIdx == numObjs_ - 1) {
      tcur_[numRowsPerObject_+1] = 0.0;
    }

    std::copy(tcur_.begin(), tcur_.end(), told_.begin());
  }


  void init(size_t numObjects, size_t nRowPerObject, size_t iterMax, NodeObjProxy objProxy) {
    numObjs_ = numObjects;
    numRowsPerObject_ = nRowPerObject;
    maxIter_ = iterMax;
    objProxy_ = objProxy;

    // Initialize the starting vector
    initializeData();
  }

};

struct NodeObj {
  using ChainSetType = vt::messaging::CollectionChainSet<vt::Index1D>;

  NodeObj(
    vt::CollectionProxy<LinearPb1DJacobi> in_proxy, std::size_t in_num_objs,
    std::size_t in_max_iter
  ) : proxy_(in_proxy),
      num_objs_(in_num_objs),
      max_iter_(in_max_iter)
  { }

  void reducedNorm(double normRes) {
    converged_ = normRes < default_tol;
    fmt::print(
      "## ITER {} >> Residual Norm = {}, conv={} \n",
      cur_iter_, normRes, converged_
    );
  }

  void setup(vt::objgroup::proxy::Proxy<NodeObj> in_proxy) {
    chains_ = std::make_unique<ChainSetType>(proxy_);
    chains_->startTasks();
    this_proxy_ = in_proxy;
  }

  void runToConvergence() {
    while (not converged_ and cur_iter_ < max_iter_) {
      vt_print(gen, "runToConvergence: cur_iter_={}, max_iter_={}\n", cur_iter_, max_iter_);

      auto xl = chains_->taskCollective("exchange left", [&](auto idx, auto t) {
        if (prev_kernel) {
          t->dependsOn(idx, *prev_kernel);
        }
        return proxy_[idx].template send<&LinearPb1DJacobi::sendLeft>();
      });

      auto xr = chains_->taskCollective("exchange right", [&](auto idx, auto t) {
        if (prev_kernel) {
          t->dependsOn(idx, *prev_kernel);
        }
        return proxy_[idx].template send<&LinearPb1DJacobi::sendRight>();
      });

      auto kernel = chains_->taskCollective("kernel", [&](auto idx, auto t) {
        if (idx.x() != 0) {
          auto left = vt::Index1D(idx.x() - 1);
          t->dependsOn(left, *xr);
        }
        if (static_cast<std::size_t>(idx.x()) != num_objs_ - 1) {
          auto right = vt::Index1D(idx.x() + 1);
          t->dependsOn(right, *xl);
        }
        return proxy_[idx].template send<&LinearPb1DJacobi::kernel>();
      });
      prev_kernel = kernel;

      if (cur_iter_++ % check_conv_freq == 0) {
        chains_->taskCollective("checkConv", [&](auto idx, auto t) {
          t->dependsOn(idx, *kernel);
          auto cb = vt::theCB()->makeBcast<&NodeObj::reducedNorm>(this_proxy_);
          return proxy_[idx].template send<&LinearPb1DJacobi::reduceMaxNorm>(cb);
        });

        chains_->waitForTasks();
        chains_->startTasks();
      }
    }

    if (not converged_) {
      chains_->waitForTasks();
      fmt::print("Maximum Number of Iterations Reached without convergence.\n");
    } else {
      fmt::print("Convergence is reached at iteration {}.\n", cur_iter_);
    }

    chains_->phaseDone();
  }

private:
  vt::CollectionProxy<LinearPb1DJacobi> proxy_;
  std::size_t num_objs_ = 0;
  std::size_t max_iter_ = 0;
  std::size_t cur_iter_ = 0;
  std::unique_ptr<ChainSetType> chains_;
  vt::task::TaskCollective<vt::Index1D>* prev_kernel = nullptr;
  bool converged_ = false;
  vt::objgroup::proxy::Proxy<NodeObj> this_proxy_;
};

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

  // Create the decomposition into objects
  using BaseIndexType = typename vt::Index1D::DenseIndexType;
  auto range = vt::Index1D(static_cast<BaseIndexType>(num_objs));

  auto col_proxy = vt::makeCollection<LinearPb1DJacobi>("jacobi1d")
    .bounds(range)
    .bulkInsert()
    .wait();

  // Object group of all nodes that take part in computation
  // Used to determine whether the computation is finished
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>(
    "jacobi1d", col_proxy, num_objs, maxIter
  );

  vt::runInEpochCollective([=]{
    col_proxy.broadcastCollective<&LinearPb1DJacobi::init>(
      num_objs, numRowsPerObject, maxIter, grp_proxy
    );
  });

  grp_proxy.get()->setup(grp_proxy);
  grp_proxy.get()->runToConvergence();

  //vt::thePhase()->nextPhaseCollective();

  vt::finalize();

  return 0;
}

/// [Jacobi1D example]
