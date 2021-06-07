/*
//@HEADER
// *****************************************************************************
//
//                                jacobi2d_vt.cc
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

#include "common/test_harness.h"

#include <vt/vrt/collection/manager.h>
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

struct LinearPb2DJacobi : vt::Collection<LinearPb2DJacobi,vt::Index2D>{

private:

  std::vector<double> tcur_, told_;
  std::vector<double> rhs_;
  size_t iter_ = 0;
  size_t msgReceived_ = 0, totalReceive_ = 0;
  size_t numObjsX_ = 1, numObjsY_ = 1;
  size_t numRowsPerObject_ = default_nrow_object;
  size_t maxIter_ = 5;
  NodeObjProxy objProxy_;

public:

  LinearPb2DJacobi()
    : tcur_(), told_(), rhs_(), iter_(0),
      msgReceived_(0), totalReceive_(0),
      numObjsX_(1), numObjsY_(1),
      numRowsPerObject_(default_nrow_object),
      maxIter_(5)
  { }


  struct BlankMsg : vt::CollectionMessage<LinearPb2DJacobi> { };

  struct LPMsg : vt::CollectionMessage<LinearPb2DJacobi> {

    size_t numXObjs = 0;
    size_t numYObjs = 0;
    size_t numIter = 0;
    NodeObjProxy objProxy;

    LPMsg() = default;

    LPMsg(const size_t nx, const size_t ny, const size_t nref, NodeObjProxy proxy)
      : numXObjs(nx), numYObjs(ny), numIter(nref), objProxy(proxy)
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

    const double normRes = msg->getConstVal();
    auto const iter_max_reached = iter_ > maxIter_;
    auto const norm_res_done = normRes < default_tol;

    if (iter_max_reached or norm_res_done) {
      // Notify all nodes that computation is finished
      objProxy_.broadcast<NodeObj::WorkFinishedMsg, &NodeObj::workFinishedHandler>();
    }
  }

  void doIteration() {

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

    //
    // Compute the maximum entries among the rows on this object
    // We do not take into account the "ghost" entries
    // as they may be "out of date".
    //

    double maxNorm = 0.0;

    for (size_t iy = 1; iy <= numRowsPerObject_; ++iy) {
      for (size_t ix = 1; ix <= numRowsPerObject_; ++ix) {
        size_t node = ix + iy * ldx;
        double val = tcur_[node];
        maxNorm = (maxNorm > std::fabs(val)) ? maxNorm : std::fabs(val);
      }
    }

    auto proxy = this->getCollectionProxy();
    auto cb = vt::theCB()->makeSend<
      LinearPb2DJacobi,ReduxMsg,&LinearPb2DJacobi::checkCompleteCB
    >(proxy(0,0));
    auto msg2 = vt::makeMessage<ReduxMsg>(maxNorm);
    proxy.reduce<vt::collective::MaxOp<double>>(msg2.get(),cb);
  }

  struct VecMsg : vt::CollectionMessage<LinearPb2DJacobi> {
    using MessageParentType = vt::CollectionMessage<LinearPb2DJacobi>;
    vt_msg_serialize_required(); // stl vector

    VecMsg() = default;
    VecMsg(IndexType const& in_index, const std::vector<double> &ref) :
      vt::CollectionMessage<LinearPb2DJacobi>(),
      from_index(in_index), val(ref)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | from_index;
      s | val;
    }

    IndexType from_index;
    std::vector<double> val;
  };

  void exchange(VecMsg *msg) {

    // Receive and treat the message from a neighboring object.

    if (this->getIndex().x() > msg->from_index.x()) {
      const size_t ldx = numRowsPerObject_ + 2;
      for (size_t jy = 0; jy < msg->val.size(); ++jy) {
        this->told_[jy*ldx] = msg->val[jy];
      }
      msgReceived_ += 1;
    }
    else if (this->getIndex().x() < msg->from_index.x()) {
      const size_t ldx = numRowsPerObject_ + 2;
      for (size_t jy = 0; jy < msg->val.size(); ++jy) {
        this->told_[numRowsPerObject_ + 1 + jy*ldx] = msg->val[jy];
      }
      msgReceived_ += 1;
    }
    else if (this->getIndex().y() > msg->from_index.y()) {
      std::copy(msg->val.begin(), msg->val.end(), this->told_.begin());
      msgReceived_ += 1;
    }
    else if (this->getIndex().y() < msg->from_index.y()) {
      std::copy(msg->val.begin(), msg->val.end(),
                &this->told_[(numRowsPerObject_ + 1)*(numRowsPerObject_ + 2)]);
      msgReceived_ += 1;
    }

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
    if (numObjsX_*numObjsY_ <= 1) {
      doIteration();
      return;
    }
    //---------------------------------------

    //
    // Routine to send information to a neighboring object
    //

    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    auto const x = idx.x();
    auto const y = idx.y();

    if (x > 0) {
      std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      for (size_t jy = 1; jy <= numRowsPerObject_; ++jy)
        tcopy[jy] = told_[1 + jy * (numRowsPerObject_ + 2)];
      proxy(x-1, y).send<VecMsg, &LinearPb2DJacobi::exchange>(idx, tcopy);
    }

    if (y > 0) {
      std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
        tcopy[jx] = told_[jx + (numRowsPerObject_ + 2)];
      proxy(x, y-1).send<VecMsg, &LinearPb2DJacobi::exchange>(idx, tcopy);
    }

    if (size_t(x) < numObjsX_ - 1) {
      std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      for (size_t jy = 1; jy <= numRowsPerObject_; ++jy) {
        tcopy[jy] = told_[numRowsPerObject_ +
                          jy * (numRowsPerObject_ + 2)];
      }
      proxy(x+1, y).send<VecMsg, &LinearPb2DJacobi::exchange>(idx, tcopy);
    }

    if (size_t(y) < numObjsY_ - 1) {
      std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
        tcopy[jx] = told_[jx + numRowsPerObject_ * (numRowsPerObject_ + 2)];
      proxy(x, y+1).send<VecMsg, &LinearPb2DJacobi::exchange>(idx, tcopy);
    }

  }


  void init() {

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

    totalReceive_ = 4;

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
      totalReceive_ -= 1;
    }

    if (idx.y() == 0) {
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx] = 0.0;
      totalReceive_ -= 1;
    }

    if (numObjsX_ == size_t(idx.x()) + 1) {
      for (size_t jy = 0; jy < ldy; ++jy)
        tcur_[jy * ldy + (ldx - 1)] = 0.0;
      totalReceive_ -= 1;
    }

    if (numObjsY_ == size_t(idx.y()) + 1) {
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx + (ldx - 1)*ldy] = 0.0;
      totalReceive_ -= 1;
    }

    std::copy(tcur_.begin(), tcur_.end(), told_.begin());

  }


  void init(LPMsg* msg) {

    numObjsX_ = msg->numXObjs;
    numObjsY_ = msg->numYObjs;
    maxIter_ = msg->numIter;
    objProxy_ = msg->objProxy;

    // Initialize the starting vector
    init();
  }

};

bool isWorkDone( vt::objgroup::proxy::Proxy<NodeObj> const& proxy){
  auto const this_node = vt::theContext()->getNode();
  return proxy[this_node].invoke<decltype(&NodeObj::isWorkFinished), &NodeObj::isWorkFinished>();
}

struct JacobiTest : vt::tests::perf::common::PerfTestHarness {};

VT_PERF_TEST(JacobiTest, jacobi2d_vt) {
  BenchmarkPhase();

  size_t numX_objs = default_num_objs;
  size_t numY_objs = default_num_objs;
  size_t maxIter = 100;

  // Object group of all nodes that take part in computation
  // Used to determine whether the computation is finished
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>();

  // Create the decomposition into objects
  using BaseIndexType = typename vt::Index2D::DenseIndexType;
  auto range = vt::Index2D(
    static_cast<BaseIndexType>(numX_objs),
    static_cast<BaseIndexType>(numY_objs));

  auto col_proxy =
    vt::theCollection()->constructCollective<LinearPb2DJacobi>(range);

  vt::runInEpochCollective(
    [col_proxy, grp_proxy, numX_objs, numY_objs, maxIter] {
      col_proxy
        .broadcastCollective<LinearPb2DJacobi::LPMsg, &LinearPb2DJacobi::init>(
          numX_objs, numY_objs, maxIter, grp_proxy);
    });

  while (!isWorkDone(grp_proxy)) {
    vt::runInEpochCollective([col_proxy] {
      col_proxy.broadcastCollective<
        LinearPb2DJacobi::BlankMsg, &LinearPb2DJacobi::doIter>();
    });

    vt::thePhase()->nextPhaseCollective();
  }
}

VT_PERF_TEST_MAIN()