/*
//@HEADER
// *****************************************************************************
//
//                                jacobi3d_vt.cc
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

#include <cstdlib>
#include <cassert>
#include <iostream>

/// [Jacobi3D example]

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
// -d^2 u / dx^2 -d^2 u / dy^2 - -d^2 u / dz^2  = f   on  [0, 1] x [0, 1] x [0, 1]
//
// with homogeneous Dirichlet condition
//
// u = 0 on the boundary of [0, 1] x [0, 1] x [0, 1]
//
// using a uniform grid with grid size
//
// 1 / ((number of objects) * (number of rows per object) + 1)
//


static constexpr std::size_t const default_nrow_object = 64;
static constexpr std::size_t const default_ncol_object = 16; // Not used at this moment
static constexpr std::size_t const default_num_objs = 4;
static constexpr double const default_tol = 1.0e-02;

struct NodeObj {
  bool is_finished_ = false;
  struct WorkFinishedMsg : vt::Message {};

  void workFinishedHandler(WorkFinishedMsg*) { is_finished_ = true; }
  bool isWorkFinished() { return is_finished_; }
};
using NodeObjProxy = vt::objgroup::proxy::Proxy<NodeObj>;

struct LinearPb3DJacobi : vt::Collection<LinearPb3DJacobi,vt::Index3D> {

private:

  std::vector<double> tcur_, told_; // 3D jx, jy*ldx, jz*ldx*ldy (X is the leading dimension)
  std::vector<double> rhs_;
  size_t iter_ = 0;
  size_t msgReceived_ = 0, totalReceive_ = 0;
  size_t numObjsX_ = 1, numObjsY_ = 1, numObjsZ_ = 1;
  size_t numRowsPerObject_ = default_nrow_object;
  // Not used at this moment. We may want to have non-qubic domain in future.
  // size_t numColsPerObject_ = default_ncol_object;
  size_t maxIter_ = 5;
  NodeObjProxy objProxy_; // Similar to MPI_Comm/MPI_Group?

public:

  LinearPb3DJacobi()
    : tcur_(), told_(), rhs_(), iter_(0),
      msgReceived_(0), totalReceive_(0),
      numObjsX_(1), numObjsY_(1), numObjsZ_(1),
      numRowsPerObject_(default_nrow_object),
      maxIter_(5)
  { }


  struct BlankMsg : vt::CollectionMessage<LinearPb3DJacobi> { };

  struct LPMsg : vt::CollectionMessage<LinearPb3DJacobi> {

    size_t numXObjs = 0;
    size_t numYObjs = 0;
    size_t numZObjs = 0;
    size_t numIter = 0;
    NodeObjProxy objProxy;

    LPMsg() = default;

    LPMsg(const size_t nx, const size_t ny, const size_t nz, const size_t nref, NodeObjProxy proxy)
      : numXObjs(nx), numYObjs(ny), numZObjs(nz), numIter(nref), objProxy(proxy)
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

    //
    //--- Copy ghost values
    //

    size_t ldx = numRowsPerObject_ + 2;
    size_t ldy = numRowsPerObject_ + 2;
    size_t ldz = numRowsPerObject_ + 2;

    for (size_t jy = 0; jy < ldy; ++jy)  // All XY at jz = 0
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx + jy * ldx] = told_[jx + jy * ldx];

    for (size_t jy = 0; jy < ldy; ++jy)  // All XY at jz = jdz-1
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx + jy * ldx + (ldz-1) * ldx * ldy] = told_[jx + jy * ldx + (ldz-1) * ldx * ldy];

    for (size_t jz = 0; jz < ldz; ++jz) //  All XZ at jy = 0
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx + jz * ldx * ldy] = told_[jx + jz * ldx *ldy];

    for (size_t jz = 0; jz < ldz; ++jz)  // All XZ at jy = ldy-1
      for (size_t jx = 0; jx < ldx; ++jx)
        tcur_[jx + (ldy-1) * ldx + jz * ldx * ldy] = told_[jx + (ldy-1) * ldx + jz * ldx * ldy];

    for (size_t jy = 0; jy < ldy; ++jy)  // All YZ at jx = 0
      for (size_t jz = 0; jz < ldz; ++jz)
        tcur_[jy * ldx + jz * ldx * ldy] = told_[jy * ldx + jz * ldx * ldy];

    for (size_t jy = 0; jy < ldy; ++jy)  // All YZ at jx = ldx-1
      for (size_t jz = 0; jz < ldz; ++jz)
        tcur_[ldx-1 + jy * ldx + jz * ldx * ldy] = told_[ldx-1 + jy * ldx + jz * ldx * ldy];

    //
    //--- Update my row values
    //

    for (size_t iz = 1; iz <= numRowsPerObject_; ++iz) {
      for (size_t iy = 1; iy <= numRowsPerObject_; ++iy) {
        for (size_t ix = 1; ix <= numRowsPerObject_; ++ix) {
          //
          //---- Jacobi iteration step for
          //---- A banded matrix for the 7-point stencil
          //---- [ 0.0  -1.0   0.0]
          //---- [-1.0]
          //---- [-1.0   6.0  -1.0]
          //---- [-1.0]
          //---- [ 0.0  -1.0   0.0]
          //---- rhs_ right hand side vector
          //
          size_t node = ix + iy * ldx + iz * ldx * ldy;
          //size_t node = ix + iy * ldx;
          tcur_[node] = (1.0/6.0) * (rhs_[node]
                             + told_[node - 1] + told_[node + 1]
                             + told_[node - ldx] + told_[node + ldx]
                             + told_[node - ldx*ldy] + told_[node + ldx*ldy]);
        }
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

    for (size_t iz = 1; iz <= numRowsPerObject_; ++iz) {
      for (size_t iy = 1; iy <= numRowsPerObject_; ++iy) {
        for (size_t ix = 1; ix <= numRowsPerObject_; ++ix) {
          size_t node = ix + iy * ldx + iz * ldx * ldy;
          //size_t node = ix + iy * ldx;
          double val = tcur_[node];
          maxNorm = (maxNorm > std::fabs(val)) ? maxNorm : std::fabs(val);
        }
      }
    }

    auto proxy = this->getCollectionProxy();
    auto cb = vt::theCB()->makeSend<
      LinearPb3DJacobi,ReduxMsg,&LinearPb3DJacobi::checkCompleteCB
    >(proxy(0,0,0));
    auto msg2 = vt::makeMessage<ReduxMsg>(maxNorm);
    proxy.reduce<vt::collective::MaxOp<double>>(msg2.get(),cb);
  }

  //
  // Message
  //
  struct VecMsg : vt::CollectionMessage<LinearPb3DJacobi> {
    using MessageParentType = vt::CollectionMessage<LinearPb3DJacobi>;
    vt_msg_serialize_required(); // stl vector

    VecMsg() = default;
    VecMsg(IndexType const& in_index, const std::vector<double> &ref, size_t const &rx, size_t const &ry, size_t const &rz) :
      vt::CollectionMessage<LinearPb3DJacobi>(),
      from_index(in_index), val(ref), nx(rx), ny(ry), nz(rz)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | from_index;
      s | val;
      s | nx;
      s | ny;
      s | nz;
    }

    IndexType from_index;
    std::vector<double> val;
    size_t nx;
    size_t ny;
    size_t nz;
  };

  void exchange(VecMsg *msg) {

    // Receive and treat the message from a neighboring object.
    if (this->getIndex().x() > msg->from_index.x()) {   // Receiving message YZ plane from X+1
      const size_t ldx = numRowsPerObject_ + 2;
      const size_t ldy = numRowsPerObject_ + 2;
      const size_t nz = msg->nz;
      const size_t ny = msg->ny;
      for (size_t jz = 0; jz < nz ; ++jz) { // msg->val_size
        for (size_t jy = 0; jy < ny ; ++jy) {
          this->told_[0+jy*ldx+ ldx*ldy*jz] =
                   msg->val[jy+jz*ny]; // Need to think
        }
      }
      msgReceived_ += 1;
    }
    else if (this->getIndex().x() < msg->from_index.x()) { // Receiving message YZ plane from X-1
      const size_t ldx = numRowsPerObject_ + 2;
      const size_t ldy = numRowsPerObject_ + 2;
      const size_t nz = msg->nz;
      const size_t ny = msg->ny;
      for (size_t jz = 0; jz < nz ; ++jz) {
        for (size_t jy = 0; jy < ny; ++jy) {
          this->told_[numRowsPerObject_ + 1 + jy*ldx + jz *ldy* ldx] = msg->val[jy+jz*ny]; // Need to think
        //this->told_[numRowsPerObject_ + 1 + jy*ldx] = msg->val[jy];
        }
      }
      msgReceived_ += 1;
    }
    else if (this->getIndex().y() > msg->from_index.y()) { // Receiving message XZ plan from Y-1
      const size_t ldy = numRowsPerObject_ + 2;
      const size_t ldx = numRowsPerObject_ + 2;
      const size_t nz = msg->nz;
      const size_t nx = msg->nx;
      // Cannot do sequential copy
      for (size_t jz = 0; jz < nz; ++jz) {
        for (size_t jx = 0; jx < nx; ++jx) {
          this->told_[jx+ldx*ldy*jz] = msg->val[jx+jz*nx];
        }
      }
      //std::copy(msg->val.begin(), msg->val.end(), this->told_.begin());
      msgReceived_ += 1;
    }
    else if (this->getIndex().y() < msg->from_index.y()) {  // Receiving message XZ plan from Y+1
      const size_t ldy = numRowsPerObject_ + 2;
      const size_t ldx = numRowsPerObject_ + 2;
      const size_t nz = msg->nz;
      const size_t nx = msg->nx;
      for (size_t jz = 0; jz < nz; ++jz) {
        for (size_t jx = 0; jx < nx; ++jx) {
          this->told_[jx+ ldx * (numRowsPerObject_ + 1) + ldx*ldy*jz] =  msg->val[jx+jz*nx];
        }
      }
      msgReceived_ += 1;
    }
    else if (this->getIndex().z() > msg->from_index.z()) { // Receiving message XY plan from Z-1
      // const size_t ldy = numRowsPerObject_ + 2;
      const size_t ldx = numRowsPerObject_ + 2;
      const size_t ny = msg->ny;
      const size_t nx = msg->nx;
      for (size_t jy = 0; jy < ny; ++jy) {
        for (size_t jx = 0; jx < nx; ++jx) {
          this->told_[jx+ jy*ldx] = msg->val[jx+jy*nx]; //
        }
      }
      msgReceived_ += 1;
    }
    else if (this->getIndex().z() < msg->from_index.z()) { // Receiving message XY plan from Z+1
      const size_t ldy = numRowsPerObject_ + 2;
      const size_t ldx = numRowsPerObject_ + 2;
      const size_t ny = msg->ny;
      const size_t nx = msg->nx;
      for (size_t jy = 0; jy < ny; ++jy) {
        for (size_t jx = 0; jx < nx; ++jx) {
          this->told_[jx+ jy*ldx + (numRowsPerObject_ + 1)*ldx*ldy ] = msg->val[jx+jy*nx];  //
        }
      }
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
    auto const z = idx.z();

    if (x > 0) {
      std::vector<double> tcopy( (numRowsPerObject_ + 2) * (numRowsPerObject_ + 2)  , 0.0); // size needs to be changed
      size_t rx = 0;
      size_t ry = numRowsPerObject_ + 2;
      size_t rz = numRowsPerObject_ + 2;
      size_t ldx = numRowsPerObject_ + 2;
      size_t ldy = numRowsPerObject_ + 2;
      // No ghost
      for (size_t jz = 1; jz <= numRowsPerObject_; ++jz)
        for (size_t jy = 1; jy <= numRowsPerObject_; ++jy)
           tcopy[jy+jz*ry] = told_[1 + jy * ldx + jz * ldx *ldy]; // put YZ plane for X=1
      proxy(x-1, y, z).send<VecMsg, &LinearPb3DJacobi::exchange>(idx, tcopy, rx, ry, rz);
    }

    if (y > 0) {
      //  std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      std::vector<double> tcopy( (numRowsPerObject_ + 2) * (numRowsPerObject_ + 2)  , 0.0); // size needs to be changed
      size_t rx = numRowsPerObject_ + 2;
      size_t ry = 0;
      size_t rz = numRowsPerObject_ + 2;
      size_t ldx = numRowsPerObject_ + 2;
      size_t ldy = numRowsPerObject_ + 2;
      // No ghost
      for (size_t jz = 1; jz <= numRowsPerObject_; ++jz)
        for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
          tcopy[jx+rx*jz] = told_[jx + ldx + jz * ldx * ldy];
      proxy(x, y-1, z).send<VecMsg, &LinearPb3DJacobi::exchange>(idx, tcopy, rx, ry, rz );
    }

    if (z > 0) {
      std::vector<double> tcopy( (numRowsPerObject_ + 2) * (numRowsPerObject_ + 2)  , 0.0); // size needs to be changed
      size_t rx = numRowsPerObject_ + 2;
      size_t ry = numRowsPerObject_ + 2;
      size_t rz = 0;
      size_t ldx = numRowsPerObject_ + 2;
      size_t ldy = numRowsPerObject_ + 2;
      // No ghost
      for (size_t jy = 1; jy <= numRowsPerObject_; ++jy)
        for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
          tcopy[jx+rx*jy] = told_[jx + jy * ldx + ldx * ldy ];
      proxy(x, y, z-1).send<VecMsg, &LinearPb3DJacobi::exchange>(idx, tcopy, rx, ry, rz);
    }

    if (size_t(x) < numObjsX_ - 1) {
      //std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      std::vector<double> tcopy( (numRowsPerObject_ + 2) * (numRowsPerObject_ + 2)  , 0.0); // size needs to be changed
      size_t rx = 0;
      size_t ry = numRowsPerObject_ + 2;
      size_t rz = numRowsPerObject_ + 2;
      size_t ldx = numRowsPerObject_ + 2;
      size_t ldy = numRowsPerObject_ + 2;

      // No ghost
      for (size_t jz = 1; jz <= numRowsPerObject_; ++jz) {
        for (size_t jy = 1; jy <= numRowsPerObject_; ++jy) {
          tcopy[jy+jz*ry] = told_[ (ldx - 2) + jy * ldx + jz * ldx * ldy ];
        }
      }
      proxy(x+1, y, z).send<VecMsg, &LinearPb3DJacobi::exchange>(idx, tcopy, rx, ry, rz);
    }

    if (size_t(y) < numObjsY_ - 1) {
      // std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      std::vector<double> tcopy( (numRowsPerObject_ + 2) * (numRowsPerObject_ + 2)  , 0.0); // size needs to be changed
      size_t rx = numRowsPerObject_ + 2;
      size_t ry = 0;
      size_t rz = numRowsPerObject_ + 2;
      size_t ldx = numRowsPerObject_ + 2;
      size_t ldy = numRowsPerObject_ + 2;
      for (size_t jz = 1; jz <= numRowsPerObject_; ++jz)
        for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
          tcopy[jx+jz*rx] = told_[jx + ( ldy - 2 ) * ldx + jz * ldx * ldy];
      proxy(x, y+1, z).send<VecMsg, &LinearPb3DJacobi::exchange>(idx, tcopy, rx, ry, rz);
    }

    if (size_t(z) < numObjsZ_ - 1) {
      // std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
      std::vector<double> tcopy( (numRowsPerObject_ + 2) * (numRowsPerObject_ + 2)  , 0.0); // size needs to be changed
      size_t rx = numRowsPerObject_ + 2;
      size_t ry = numRowsPerObject_ + 2;
      size_t rz = 0;
      size_t ldx = numRowsPerObject_ + 2;
      size_t ldy = numRowsPerObject_ + 2;
      size_t ldz = numRowsPerObject_ + 2;
      for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
        for (size_t jy = 1; jy <= numRowsPerObject_; ++jy)
          tcopy[jx + jy *rx] = told_[ jx + jy * ldx + (ldz - 2 ) * ldx * ldy];
      proxy(x, y, z+1).send<VecMsg, &LinearPb3DJacobi::exchange>(idx, tcopy, rx, ry, rz);
    }

  }


  void init() {

    //--- Each object will work with (numRowsPerObject_ + 2) unknowns
    //--- or (numRowsPerObject_ + 2) rows of the matrix
    size_t ldx = numRowsPerObject_ + 2, ldy = ldx, ldz = ldx;

    size_t vecSize = ldx * ldy * ldz;
    tcur_.assign(vecSize, 0.0);
    told_.assign(vecSize, 0.0);
    rhs_.assign(vecSize, 0.0);

    //
    // Set the initial vector to the values of
    // a "high-frequency" function
    //

    double hx = 1.0 / (numRowsPerObject_ * numObjsX_ + 1.0);
    double hy = 1.0 / (numRowsPerObject_ * numObjsY_ + 1.0);
    double hz = 1.0 / (numRowsPerObject_ * numObjsZ_ + 1.0);
    // Need 3 values for max
    size_t maxNObjs = (size_t) std::max(numObjsX_, numObjsY_);
    maxNObjs = (maxNObjs < numObjsY_ ) ? numObjsY_ : maxNObjs;

    int nf = 3 * int(numRowsPerObject_ * maxNObjs + 1) / 6;

    auto idx = this->getIndex();

    for (size_t iz = 0; iz < ldz; ++iz) {
      for (size_t iy = 0; iy < ldy; ++iy) {
        for (size_t ix = 0; ix < ldx; ++ix) {
          double x0 = ( numRowsPerObject_ * idx.x() + ix) * hx;
          double y0 = ( numRowsPerObject_ * idx.y() + iy) * hy;
          double z0 = ( numRowsPerObject_ * idx.z() + iz) * hz;
          size_t node = ix + iy * ldx + iz * ldx * ldy;
          tcur_[node] = sin(nf * M_PI * (x0 * x0 + y0 * y0 + z0 *z0));
        }
      }
    }

    totalReceive_ = 6; // 6 directions

    //
    //--- The unknowns correspond to the interior nodes
    //--- of a regular orthogonal grid on [0, 1] x [0, 1] x [0, 1]
    //--- The total number of grid points in X-direction is
    //--- (numRowsPerObject_ * numObjsX_) + 2
    //--- The total number of grid points in Y-direction is
    //--- (numRowsPerObject_ * numObjsY_) + 2
    //--- The total number of grid points in Z-direction is
    //--- (numRowsPerObject_ * numObjsY_) + 2
    //
    if (idx.x() == 0) {
      for (size_t jz = 0; jz < ldz; ++jz)
        for (size_t jy = 0; jy < ldy; ++jy)
          tcur_[jy * ldx + jz * ldx * ldy] = 0.0;
      totalReceive_ -= 1;
    }

    if (idx.y() == 0) {
      for (size_t jz = 0; jz < ldz; ++jz)
        for (size_t jx = 0; jx < ldx; ++jx)
          tcur_[jx + jz * ldx * ldy] = 0.0;
      totalReceive_ -= 1;
    }

    if (idx.z() == 0) {
      for (size_t jy = 0; jy < ldy; ++jy)
        for (size_t jx = 0; jx < ldx; ++jx)
          tcur_[jx + jy * ldx] = 0.0;
      totalReceive_ -= 1;
    }


    if (numObjsX_ == size_t(idx.x()) + 1) {
      for (size_t jz = 0; jz < ldz; ++jz)
        for (size_t jy = 0; jy < ldy; ++jy)
          tcur_[(ldx - 1) + jy * ldx + jz * ldx * ldy] = 0.0;
      totalReceive_ -= 1;
    }

    if (numObjsY_ == size_t(idx.y()) + 1) {
      for (size_t jz = 0; jz < ldz; ++jz)
        for (size_t jx = 0; jx < ldx; ++jx)
          tcur_[jx + (ldy - 1)*ldx + jz * ldx * ldy] = 0.0;
      totalReceive_ -= 1;
    }

    if (numObjsZ_ == size_t(idx.z()) + 1) {
      for (size_t jy = 0; jy < ldy; ++jy)
        for (size_t jx = 0; jx < ldx; ++jx)
          tcur_[jx + jy * ldx + (ldz -1 ) * ldx * ldy] = 0.0;
      totalReceive_ -= 1;
    }


    std::copy(tcur_.begin(), tcur_.end(), told_.begin());

  }


  void init(LPMsg* msg) {

    numObjsX_ = msg->numXObjs;
    numObjsY_ = msg->numYObjs;
    numObjsZ_ = msg->numYObjs;
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

int main(int argc, char** argv) {

  // Need the default value??
  size_t numX_objs = default_num_objs;
  size_t numY_objs = default_num_objs;
  size_t numZ_objs = default_num_objs;
  size_t maxIter = 10;

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
    if (argc == 4) {
      numX_objs = (size_t) strtol(argv[1], nullptr, 10);
      numY_objs = (size_t) strtol(argv[2], nullptr, 10);
      numZ_objs = (size_t) strtol(argv[3], nullptr, 10);
    }
    else if (argc == 5) {
      numX_objs = (size_t) strtol(argv[1], nullptr, 10);
      numY_objs = (size_t) strtol(argv[2], nullptr, 10);
      numZ_objs = (size_t) strtol(argv[3], nullptr, 10);
      maxIter = (size_t) strtol(argv[4], nullptr, 10);
    }
    else {
      fmt::print(
        stderr, "usage: {} <num-objects-X-direction> <num-objects-Y-direction> <num-objects-Z-direction> <maxiter>\n",
        name
      );
      return 1;
    }
  }

  /* --- Print information about the simulation */

  if (this_node == 0) {
    fmt::print(
      stdout, "\n - Solve the linear system for the Laplacian with homogeneous Dirichlet"
      " on [0, 1] x [0, 1] x [0, 1]\n"
    );
    fmt::print(stdout, " - Second-order centered finite difference\n");
    fmt::print(
      stdout, " - Uniform grid with ({} x {} x {} = {}) points in the x-direction and ({} x {} x {}  = {}) points in the y-direction  ({} x {} x {}  = {}) points in the z-direction\n",
      numX_objs, default_nrow_object, default_nrow_object, numX_objs * default_nrow_object * default_nrow_object,
      numY_objs, default_nrow_object, default_nrow_object, numY_objs * default_nrow_object * default_nrow_object,
      numZ_objs, default_nrow_object, default_nrow_object, numZ_objs * default_nrow_object * default_nrow_object
    );
    fmt::print(stdout, " - Maximum number of iterations {}\n", maxIter);
    fmt::print(stdout, " - Convergence tolerance {}\n", default_tol);
    fmt::print(stdout, "\n");
  }

  // Object group of all nodes that take part in computation
  // Used to determine whether the computation is finished
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>();

  // Create the decomposition into objects
  using BaseIndexType = typename vt::Index3D::DenseIndexType;
  auto range = vt::Index3D(
    static_cast<BaseIndexType>(numX_objs),
    static_cast<BaseIndexType>(numY_objs),
    static_cast<BaseIndexType>(numZ_objs)
  );

  // Need 3D partioning
  auto col_proxy = vt::makeCollection<LinearPb3DJacobi>()
    .bounds(range)
    .bulkInsert()
    .wait();

  vt::runInEpochCollective([col_proxy, grp_proxy, numX_objs, numY_objs, numZ_objs, maxIter] {
    col_proxy.broadcastCollective<LinearPb3DJacobi::LPMsg, &LinearPb3DJacobi::init>(
      numX_objs, numY_objs, numZ_objs, maxIter, grp_proxy
    );
  });

  while (!isWorkDone(grp_proxy)) {
    vt::runInEpochCollective([col_proxy] {
      col_proxy.broadcastCollective<
        LinearPb3DJacobi::BlankMsg, &LinearPb3DJacobi::doIter>();
    });

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;

}
/// [Jacobi3D example]
