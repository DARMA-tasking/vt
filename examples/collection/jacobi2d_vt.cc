/*
//@HEADER
// *****************************************************************************
//
//                                jacobi2d_vt.cc
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
#include <vt/messaging/collection_chain_set.h>

#include <cstdlib>
#include <cassert>
#include <iostream>

/// [Jacobi2D example]

//
// This code applies a few steps of the Jacobi iteration to
// the linear system  A x = 0
// where is a banded symmetric positive definite matrix.
// The initial guess for x is a made-up non-zero vector.
// The exact solution is the vector 0.
//
// The matrix A is square and invertible.
// The number of rows is ((number of objects) * (number of rows per object))
//
// Such a matrix A is obtained when using 2nd-order finite difference
// for discretizing
//
// -d^2 u / dx^2 -d^2 u / dy^2 = f   on  [0, 1] x [0, 1]
//
// with homogeneous Dirichlet condition
//
// u = 0 on the boundary of [0, 1] x [0, 1]
//
// using a uniform grid with grid size
//
// 1 / ((number of objects) * (number of rows per object) + 1)
//


static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static std::size_t check_conv_freq = 1;
static constexpr double const default_tol = 1.0e-02;

struct NodeObj;
using NodeObjProxy = vt::objgroup::proxy::Proxy<NodeObj>;

struct LinearPb2DJacobi : vt::Collection<LinearPb2DJacobi,vt::Index2D> {
private:
  std::vector<double> tcur_, told_;
  std::vector<double> rhs_;
  size_t iter_ = 0;
  size_t numObjsX_ = 1, numObjsY_ = 1;
  size_t numRowsPerObject_ = default_nrow_object;
  size_t maxIter_ = 5;
  NodeObjProxy objProxy_;

public:

  LinearPb2DJacobi()
    : tcur_(), told_(), rhs_(), iter_(0),
      numObjsX_(1), numObjsY_(1),
      numRowsPerObject_(default_nrow_object),
      maxIter_(5)
  { }


  void kernel() {

    //
    //--- Copy ghost values
    //

    size_t ldx = numRowsPerObject_ + 2;
    size_t ldy = numRowsPerObject_ + 2;

    for (size_t jx = 0; jx < ldx; ++jx)
      tcur_[jx] = told_[jx];

    for (size_t jx = 0; jx < ldx; ++jx)
      tcur_[jx + (ldy-1) * ldx] = told_[jx + (ldy-1) * ldx];

    for (size_t jy = 0; jy < ldy; ++jy)
      tcur_[jy * ldx] = told_[jy * ldx];

    for (size_t jy = 0; jy < ldy; ++jy)
      tcur_[ldx-1 + jy * ldx] = told_[ldx-1 + jy * ldx];

    //
    //--- Update my row values
    //

    for (size_t iy = 1; iy <= numRowsPerObject_; ++iy) {
      for (size_t ix = 1; ix <= numRowsPerObject_; ++ix) {
        //
        //---- Jacobi iteration step for
        //---- A banded matrix for the 5-point stencil
        //---- [ 0.0  -1.0   0.0]
        //---- [-1.0   4.0  -1.0]
        //---- [ 0.0  -1.0   0.0]
        //---- rhs_ right hand side vector
        //
        size_t node = ix + iy * ldx;
        tcur_[node] = 0.25 * (rhs_[node]
                              + told_[node - 1] + told_[node + 1]
                              + told_[node - ldx] + told_[node + ldx]);
      }
    }

    iter_ += 1;

    std::copy(tcur_.begin(), tcur_.end(), told_.begin());
  }

  double computeMaxNorm() {
    // Compute the maximum entries among the rows on this object We do not take
    // into account the "ghost" entries as they may be "out of date".
    double maxNorm = 0.0;
    size_t ldx = numRowsPerObject_ + 2;
    for (size_t iy = 1; iy <= numRowsPerObject_; ++iy) {
      for (size_t ix = 1; ix <= numRowsPerObject_; ++ix) {
        size_t node = ix + iy * ldx;
        double val = tcur_[node];
        maxNorm = (maxNorm > std::fabs(val)) ? maxNorm : std::fabs(val);
      }
    }
    return maxNorm;
  }

  void reduceMaxNorm(vt::Callback<double> cb) {
    auto proxy = this->getCollectionProxy();
    proxy.reduce<vt::collective::MaxOp>(cb, computeMaxNorm());
  }

  void exchange(vt::Index2D from_index, std::vector<double> val, std::size_t in_iter) {
    // Receive and treat the message from a neighboring object.
    vtAssert(iter_ == in_iter, "iters should match");
    if (this->getIndex().x() > from_index.x()) {
      const size_t ldx = numRowsPerObject_ + 2;
      for (size_t jy = 0; jy < val.size(); ++jy) {
        this->told_[jy*ldx] = val[jy];
      }
    }
    else if (this->getIndex().x() < from_index.x()) {
      const size_t ldx = numRowsPerObject_ + 2;
      for (size_t jy = 0; jy < val.size(); ++jy) {
        this->told_[numRowsPerObject_ + 1 + jy*ldx] = val[jy];
      }
    }
    else if (this->getIndex().y() > from_index.y()) {
      std::copy(val.begin(), val.end(), this->told_.begin());
    }
    else if (this->getIndex().y() < from_index.y()) {
      std::copy(val.begin(), val.end(),
                &this->told_[(numRowsPerObject_ + 1)*(numRowsPerObject_ + 2)]);
    }
  }

  void sendNeighbor(std::tuple<int, int> direction) {
    using Tuple = std::tuple<int, int>;

    //
    // Routine to send information to a neighboring object
    //

    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    auto const x = idx.x();
    auto const y = idx.y();

    if (direction == Tuple{-1, 0}) {
      if (x > 0) {
        std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
        for (size_t jy = 1; jy <= numRowsPerObject_; ++jy)
          tcopy[jy] = told_[1 + jy * (numRowsPerObject_ + 2)];
        proxy(x-1, y).send<&LinearPb2DJacobi::exchange>(idx, tcopy, iter_);
      }
    }

    if (direction == Tuple{0, -1}) {
      if (y > 0) {
        std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
        for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
          tcopy[jx] = told_[jx + (numRowsPerObject_ + 2)];
        proxy(x, y-1).send<&LinearPb2DJacobi::exchange>(idx, tcopy, iter_);
      }
    }

    if (direction == Tuple{1, 0}) {
      if (size_t(x) < numObjsX_ - 1) {
        std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
        for (size_t jy = 1; jy <= numRowsPerObject_; ++jy) {
          tcopy[jy] = told_[numRowsPerObject_ +
                            jy * (numRowsPerObject_ + 2)];
        }
        proxy(x+1, y).send<&LinearPb2DJacobi::exchange>(idx, tcopy, iter_);
      }
    }

    if (direction == Tuple{0, 1}) {
      if (size_t(y) < numObjsY_ - 1) {
        std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
        for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
          tcopy[jx] = told_[jx + numRowsPerObject_ * (numRowsPerObject_ + 2)];
        proxy(x, y+1).send<&LinearPb2DJacobi::exchange>(idx, tcopy, iter_);
      }
    }
  }

  void initializeData() {
    //--- Each object will work with (numRowsPerObject_ + 2) unknowns
    //--- or (numRowsPerObject_ + 2) rows of the matrix
    size_t ldx = numRowsPerObject_ + 2, ldy = ldx;

    size_t vecSize = ldx * ldy;
    tcur_.assign(vecSize, 0.0);
    told_.assign(vecSize, 0.0);
    rhs_.assign(vecSize, 0.0);

    //
    // Set the initial vector to the values of
    // a "high-frequency" function
    //

    double hx = 1.0 / (numRowsPerObject_ * numObjsX_ + 1.0);
    double hy = 1.0 / (numRowsPerObject_ * numObjsY_ + 1.0);

    size_t maxNObjs = (size_t) std::max(numObjsX_, numObjsY_);
    int nf = 3 * int(numRowsPerObject_ * maxNObjs + 1) / 4;

    auto idx = this->getIndex();

    for (size_t iy = 0; iy < ldy; ++iy) {
      for (size_t ix = 0; ix < ldx; ++ix) {
        double x0 = ( numRowsPerObject_ * idx.x() + ix) * hx;
        double y0 = ( numRowsPerObject_ * idx.y() + iy) * hy;
        size_t node = ix + iy * ldx;
        tcur_[node] = sin(nf * M_PI * (x0 * x0 + y0 * y0));
      }
    }

    //
    //--- The unknowns correspond to the interior nodes
    //--- of a regular orthogonal grid on [0, 1] x [0, 1]
    //--- The total number of grid points in X-direction is
    //--- (numRowsPerObject_ * numObjsX_) + 2
    //--- The total number of grid points in Y-direction is
    //--- (numRowsPerObject_ * numObjsY_) + 2
    //
    if (idx.x() == 0) {
      for (size_t jy = 0; jy < ldy; ++jy)
        tcur_[jy * ldy] = 0.0;
    }

    if (idx.y() == 0) {
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx] = 0.0;
    }

    if (numObjsX_ == size_t(idx.x()) + 1) {
      for (size_t jy = 0; jy < ldy; ++jy)
        tcur_[jy * ldy + (ldx - 1)] = 0.0;
    }

    if (numObjsY_ == size_t(idx.y()) + 1) {
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx + (ldx - 1)*ldy] = 0.0;
    }
    std::copy(tcur_.begin(), tcur_.end(), told_.begin());
  }


  void init(
    size_t numXObjs, size_t numYObjs, size_t nRowPerObject, size_t iterMax,
    NodeObjProxy objProxy
  ) {
    numRowsPerObject_ = nRowPerObject;
    numObjsX_ = numXObjs;
    numObjsY_ = numYObjs;
    maxIter_ = iterMax;
    objProxy_ = objProxy;

    // Initialize the starting vector
    initializeData();
  }

};

struct NodeObj {
  using ChainSetType = vt::messaging::CollectionChainSet<vt::Index2D>;

  NodeObj(
    vt::CollectionProxy<LinearPb2DJacobi> in_proxy, std::size_t in_num_objs_x,
    std::size_t in_num_objs_y, std::size_t in_max_iter
  ) : proxy_(in_proxy),
      num_objs_x_(in_num_objs_x),
      num_objs_y_(in_num_objs_y),
      max_iter_(in_max_iter)
  { }

  void reducedNorm(double normRes) {
    converged_ = normRes < default_tol;
    if (vt::theContext()->getNode() == 0) {
      fmt::print(
        "## ITER {} >> Residual Norm = {}, conv={} \n",
        cur_iter_, normRes, converged_
      );
    }
  }

  void setup(vt::objgroup::proxy::Proxy<NodeObj> in_proxy) {
    chains_ = std::make_unique<ChainSetType>(proxy_);
    this_proxy_ = in_proxy;
  }

  void runToConvergence() {
    using Tuple = std::tuple<int, int>;

    vt::task::TaskCollective<vt::Index2D>* prev_kernel = nullptr;

    auto iteration = chains_->createTaskRegion([&]{
      std::array<Tuple, 4> dirs = {
        Tuple{-1, 0}, Tuple{0,-1}, Tuple{1,0}, Tuple{0,1}
      };
      std::map<std::tuple<int, int>, vt::task::TaskCollective<vt::Index2D>*> dep_dir;

      for (auto const& [x, y] : dirs) {
        auto dep = chains_->taskCollective("exchange", [&](auto idx, auto t) {
          if (prev_kernel) {
            t->dependsOn(idx, prev_kernel);

            vt::Index2D idx_dir{idx.x() + x, idx.y() + y};

            if (idx_dir.x() >= 0 and idx_dir.x() < int(num_objs_x_) and
                idx_dir.y() >= 0 and idx_dir.y() < int(num_objs_y_)) {
              t->dependsOn(idx_dir, prev_kernel);
            }
          }
          return proxy_[idx].template send<&LinearPb2DJacobi::sendNeighbor>(Tuple{x,y});
        });
        dep_dir[Tuple{x,y}] = dep;
      }

      prev_kernel = chains_->taskCollective("kernel", [&](auto idx, auto t) {
        if (idx.x() != 0) {
          t->dependsOn(vt::Index2D(idx.x() - 1, idx.y()), dep_dir[Tuple{1,0}]);
        }
        if (idx.y() != 0) {
          t->dependsOn(vt::Index2D(idx.x(), idx.y() - 1), dep_dir[Tuple{0,1}]);
        }
        if (static_cast<std::size_t>(idx.x()) != num_objs_x_ - 1) {
          t->dependsOn(vt::Index2D(idx.x() + 1, idx.y()), dep_dir[Tuple{-1,0}]);
        }
        if (static_cast<std::size_t>(idx.y()) != num_objs_y_ - 1) {
          t->dependsOn(vt::Index2D(idx.x(), idx.y() + 1), dep_dir[Tuple{0,-1}]);
        }
        return proxy_[idx].template send<&LinearPb2DJacobi::kernel>();
      });
    });

    while (not converged_ and cur_iter_ < max_iter_) {
      iteration->enqueueTasks();

      if (cur_iter_++ % check_conv_freq == 0) {
        chains_->taskCollective("checkConv", [&](auto idx, auto t) {
          t->dependsOn(idx, prev_kernel);
          auto cb = vt::theCB()->makeBcast<&NodeObj::reducedNorm>(this_proxy_);
          return proxy_[idx].template send<&LinearPb2DJacobi::reduceMaxNorm>(cb);
        });

        iteration->waitCollective();
        vt::thePhase()->nextPhaseCollective();
      }
    }

    iteration->waitCollective();

    if (vt::theContext()->getNode() == 0) {
      if (not converged_) {
        fmt::print("Maximum Number of Iterations Reached without convergence.\n");
      } else {
        fmt::print("Convergence is reached at iteration {}.\n", cur_iter_);
      }
    }

    chains_->phaseDone();
    chains_ = nullptr;
  }

private:
  vt::CollectionProxy<LinearPb2DJacobi> proxy_;
  std::size_t num_objs_x_ = 0;
  std::size_t num_objs_y_ = 0;
  std::size_t max_iter_ = 0;
  std::size_t cur_iter_ = 0;
  std::unique_ptr<ChainSetType> chains_;
  bool converged_ = false;
  vt::objgroup::proxy::Proxy<NodeObj> this_proxy_;
};

int main(int argc, char** argv) {

  size_t numX_objs = default_num_objs;
  size_t numY_objs = default_num_objs;
  size_t maxIter = 10;
  size_t numRows = default_nrow_object;

  std::string name(argv[0]);

  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  if (argc == 1) {
    if (this_node == 0) {
      fmt::print(
        stderr, "{}: using default arguments since none provided\n", name
      );
    }
  } else {
    if (argc == 3) {
      numX_objs = (size_t) strtol(argv[1], nullptr, 10);
      numY_objs = (size_t) strtol(argv[2], nullptr, 10);
    }
    else if (argc == 4) {
      numX_objs = (size_t) strtol(argv[1], nullptr, 10);
      numY_objs = (size_t) strtol(argv[2], nullptr, 10);
      maxIter = (size_t) strtol(argv[3], nullptr, 10);
    }
    else if (argc == 5 or argc == 6) {
      numX_objs = (size_t) strtol(argv[1], nullptr, 10);
      numY_objs = (size_t) strtol(argv[2], nullptr, 10);
      maxIter = (size_t) strtol(argv[3], nullptr, 10);
      numRows = (size_t) strtol(argv[4], nullptr, 10);
      if (argc == 6) {
        check_conv_freq = (size_t) strtol(argv[5], nullptr, 10);
      }
    }
    else {
      fmt::print(
        stderr,
        "usage: {} <num-objects-X-direction> <num-objects-Y-direction>"
        " <maxiter> <num_rows>\n",
        name
      );
      return 1;
    }
  }

  /* --- Print information about the simulation */
  if (this_node == 0) {
    fmt::print(
      stdout, "\n - Solve the linear system for the Laplacian with homogeneous Dirichlet"
      " on [0, 1] x [0, 1]\n"
    );
    fmt::print(stdout, " - Second-order centered finite difference\n");
    fmt::print(
      stdout, " - Uniform grid with ({} x {} = {}) points in the x-direction and "
      " ({} x {} = {}) points in the y-direction\n",
      numX_objs, default_nrow_object, numX_objs * default_nrow_object,
      numY_objs, default_nrow_object, numY_objs * default_nrow_object
    );
    fmt::print(stdout, " - Maximum number of iterations {}\n", maxIter);
    fmt::print(stdout, " - Convergence tolerance {}\n", default_tol);
    fmt::print(stdout, "\n");
  }

  // Create the decomposition into objects
  using BaseIndexType = typename vt::Index2D::DenseIndexType;
  auto range = vt::Index2D(
    static_cast<BaseIndexType>(numX_objs),
    static_cast<BaseIndexType>(numY_objs)
  );

  auto col_proxy = vt::makeCollection<LinearPb2DJacobi>("jacobi2d")
    .bounds(range)
    .bulkInsert()
    .wait();

  // Object group of all nodes that take part in computation
  // Used to determine whether the computation is finished
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>(
    "jacobi2d", col_proxy, numX_objs, numY_objs, maxIter
  );

  vt::runInEpochCollective([=] {
    col_proxy.broadcastCollective<&LinearPb2DJacobi::init>(
      numX_objs, numY_objs, numRows, maxIter, grp_proxy
    );
  });

  grp_proxy.get()->setup(grp_proxy);
  grp_proxy.get()->runToConvergence();

  vt::finalize();

  return 0;

}
/// [Jacobi2D example]
