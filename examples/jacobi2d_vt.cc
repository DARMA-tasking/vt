/*
//@HEADER
// ************************************************************************
//
//                          jacobi2d_vt.cc
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

#include "vt/transport.h"

#include <cstdlib>
#include <cassert>
#include <iostream>


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



using namespace ::vt;


static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr double const default_tol = 1.0e-02;


struct LinearPb2DJacobi : vt::Collection<LinearPb2DJacobi,Index2D> {

private:

    Index2D idx_;
    std::vector<double> tcur_, told_;
    std::vector<double> rhs_;
    size_t iter_;
    size_t msgReceived_, totalReceive_;
    size_t numObjsX_ = 1, numObjsY_ = 1;
    size_t numRowsPerObject_ = default_nrow_object;
    size_t maxIter_ = 5;
    double normRes_ = 0.0;

public:

    LinearPb2DJacobi() = default;

    explicit LinearPb2DJacobi(Index2D in_idx)
            : vt::Collection<LinearPb2DJacobi,Index2D>(), idx_(in_idx),
            tcur_(), told_(), rhs_(), iter_(0),
            msgReceived_(0), totalReceive_(0),
              numObjsX_(1), numObjsY_(1),
              numRowsPerObject_(default_nrow_object),
              maxIter_(5), normRes_(0.0)
    { }


    struct BlankMsg : vt::CollectionMessage<LinearPb2DJacobi> {
        BlankMsg() = default;
    };


    struct LPMsg : vt::CollectionMessage<LinearPb2DJacobi> {

        size_t numXObjs;
        size_t numYObjs;
        size_t numIter;

        LPMsg() = default;

        LPMsg(const size_t nx, const size_t ny, const size_t nref) :
                CollectionMessage<LinearPb2DJacobi>(),
                numXObjs(nx), numYObjs(ny), numIter(nref)
        { }

    };


    void checkCompletion() {

        if ((iter_ <= maxIter_) and (normRes_ >= default_tol)) {
            std::cout << " ### ITER " << iter_ ;
            std::cout << " >> Residual Norm = " << normRes_ << "\n";
            auto proxy = this->getCollectionProxy();
            auto loopMsg = makeSharedMessage< LinearPb2DJacobi::BlankMsg >();
            proxy.broadcast< LinearPb2DJacobi::BlankMsg,
                    &LinearPb2DJacobi::sendInfo>(loopMsg);
        }
        else if (iter_ > maxIter_) {
            std::cout << "\n Maximum Number of Iterations Reached. \n\n";
        }
        else {
            std::cout << "\n Max-Norm Residual Reduced by " << default_tol;
            std::cout << "\n\n";
        }

    }


    struct ReduxMsg : vt::collective::ReduceTMsg<double> {

        LinearPb2DJacobi *linearpb_;

        explicit ReduxMsg(double in_val, LinearPb2DJacobi *pbref)
                : ReduceTMsg<double>(in_val), linearpb_(pbref)
        { }
    };


    void setNormResidual(double val) { normRes_ = val; };


    struct SetNorm {
        void operator()(ReduxMsg* msg) {
            if (msg->isRoot()) {
                msg->linearpb_->setNormResidual(msg->getConstVal());
                msg->linearpb_->checkCompletion();
            }
        }
    };


    void getNorm(BlankMsg *msg) {

        //
        // Compute the maximum entries among the rows on this object
        // We do not take into account the "ghost" entries
        // as they may be "out of date".
        //

        double maxNorm = 0.0;
        size_t ldx = numRowsPerObject_ + 2;

        for (size_t iy = 1; iy <= numRowsPerObject_; ++iy) {
            for (size_t ix = 1; ix <= numRowsPerObject_; ++ix) {
                size_t node = ix + iy * ldx;
                double val = tcur_[node];
                maxNorm = (maxNorm > abs(val)) ? maxNorm : abs(val);
            }
        }

        auto proxy = this->getCollectionProxy();
        auto msg2 = makeSharedMessage<ReduxMsg>(maxNorm, this);
        theCollection()->reduceMsg<
                LinearPb2DJacobi,
                ReduxMsg,
                ReduxMsg::template msgHandler<
                        ReduxMsg, collective::MaxOp<double>, SetNorm >
                   >(proxy, msg2);

    }


    struct DoneStepMsg : vt::collective::ReduceTMsg<int> {

        LinearPb2DJacobi *linearpb_;

        explicit DoneStepMsg(int in_val, LinearPb2DJacobi *pbref)
                : ReduceTMsg<int>(in_val), linearpb_(pbref)
        { }
    };


    struct CountDone {
        void operator()(DoneStepMsg* msg) {
            if (msg->isRoot()) {
                auto proxy = msg->linearpb_->getCollectionProxy();
                auto normMsg = makeSharedMessage< LinearPb2DJacobi::BlankMsg >();
                proxy.broadcast< LinearPb2DJacobi::BlankMsg,
                        &LinearPb2DJacobi::getNorm >(normMsg);
            }
        }
    };


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

        auto proxy = this->getCollectionProxy();
        auto msg = makeSharedMessage<DoneStepMsg>(1, this);
        theCollection()->reduceMsg<
                LinearPb2DJacobi,
                DoneStepMsg,
                DoneStepMsg::template msgHandler<
                        DoneStepMsg, collective::PlusOp<int>, CountDone >
               >(proxy, msg);

    }

    struct VecMsg : vt::CollectionMessage<LinearPb2DJacobi> {

        IndexType from_index;
        std::vector<double> val;

        VecMsg() = default;
        VecMsg(IndexType const& in_index, const std::vector<double> &ref) :
                vt::CollectionMessage<LinearPb2DJacobi>(),
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

        if (this->idx_.x() > msg->from_index.x()) {
            size_t ldx = numRowsPerObject_ + 2;
            size_t ldy = ldx;
            for (size_t jy = 0; jy < msg->val.size(); ++jy) {
                this->told_[jy*ldx] = msg->val[jy];
            }
            msgReceived_ += 1;
        }
        else if (this->idx_.x() < msg->from_index.x()) {
            size_t ldx = numRowsPerObject_ + 2;
            size_t ldy = ldx;
            for (size_t jy = 0; jy < msg->val.size(); ++jy) {
                this->told_[numRowsPerObject_ + 1 + jy*ldx] = msg->val[jy];
            }
            msgReceived_ += 1;
        }
        else if (this->idx_.y() > msg->from_index.y()) {
            std::copy(msg->val.begin(), msg->val.end(), this->told_.begin());
            msgReceived_ += 1;
        }
        else if (this->idx_.y() < msg->from_index.y()) {
            std::copy(msg->val.begin(), msg->val.end(),
                      &this->told_[(numRowsPerObject_ + 1)*(numRowsPerObject_ + 2)]);
            msgReceived_ += 1;
        }

        if (msgReceived_ == totalReceive_) {
            msgReceived_ = 0;
            doIteration();
        }

    }


    void sendInfo(BlankMsg *msg) {

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

        if (idx_.x() > 0) {
            std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
            for (size_t jy = 1; jy <= numRowsPerObject_; ++jy)
                tcopy[jy] = told_[1 + jy * (numRowsPerObject_ + 2)];
            auto leftX = vt::makeSharedMessage< VecMsg >(idx_, tcopy);
            theCollection()->sendMsg< LinearPb2DJacobi::VecMsg,
                    &LinearPb2DJacobi::exchange > (proxy(idx_.x()-1, idx_.y()),
                            leftX);
        }

        if (idx_.y() > 0) {
            std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
            for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
                tcopy[jx] = told_[jx + (numRowsPerObject_ + 2)];
            auto bottomY = vt::makeSharedMessage< VecMsg >(idx_, tcopy);
            theCollection()->sendMsg< LinearPb2DJacobi::VecMsg,
                    &LinearPb2DJacobi::exchange > (proxy(idx_.x(), idx_.y()-1),
                            bottomY);
        }

        if (idx_.x() < numObjsX_ - 1) {
            std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
            for (size_t jy = 1; jy <= numRowsPerObject_; ++jy) {
                tcopy[jy] = told_[numRowsPerObject_ +
                                  jy * (numRowsPerObject_ + 2)];
            }
            auto rightX = vt::makeSharedMessage< VecMsg >(idx_, tcopy);
            theCollection()->sendMsg< LinearPb2DJacobi::VecMsg,
                    &LinearPb2DJacobi::exchange > (proxy(idx_.x()+1, idx_.y()),
                            rightX);
        }

        if (idx_.y() < numObjsY_ - 1) {
            std::vector<double> tcopy(numRowsPerObject_ + 2, 0.0);
            for (size_t jx = 1; jx <= numRowsPerObject_; ++jx)
                tcopy[jx] = told_[jx + numRowsPerObject_ * (numRowsPerObject_ + 2)];
            auto topY = vt::makeSharedMessage< VecMsg >(idx_, tcopy);
            theCollection()->sendMsg< LinearPb2DJacobi::VecMsg,
                    &LinearPb2DJacobi::exchange > (proxy(idx_.x(), idx_.y()+1),
                            topY);
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

        for (size_t iy = 0; iy < ldy; ++iy) {
            for (size_t ix = 0; ix < ldx; ++ix) {
                double x0 = ( numRowsPerObject_ * idx_.x() + ix) * hx;
                double y0 = ( numRowsPerObject_ * idx_.y() + iy) * hy;
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
        if (idx_.x() == 0) {
            for (int jy = 0; jy < ldy; ++jy)
                tcur_[jy * ldy] = 0.0;
            totalReceive_ -= 1;
        }

        if (idx_.y() == 0) {
            for (int jx = 0; jx < ldx; ++jx)
                tcur_[jx] = 0.0;
            totalReceive_ -= 1;
        }

        if (idx_.x() == numObjsX_ - 1) {
            for (int jy = 0; jy < ldy; ++jy)
                tcur_[jy * ldy + (ldx - 1)] = 0.0;
            totalReceive_ -= 1;
        }

        if (idx_.y() == numObjsY_ - 1) {
            for (int jx = 0; jx < ldx; ++jx)
                tcur_[jx + (ldx - 1)*ldy] = 0.0;
            totalReceive_ -= 1;
        }

        std::copy(tcur_.begin(), tcur_.end(), told_.begin());

    }


    void solve(LPMsg* msg) {

        numObjsX_ = msg->numXObjs;
        numObjsY_ = msg->numYObjs;
        maxIter_ = msg->numIter;

        // Initialize the starting vector
        init();

        // Start the algorithm with a neighbor-to-neighbor communication
        if ((idx_.x() == 0) and (idx_.y() == 0)){
            auto proxy = this->getCollectionProxy();
            auto loopMsg = makeSharedMessage< LinearPb2DJacobi::BlankMsg >();
            proxy.broadcast< LinearPb2DJacobi::BlankMsg,
                            &LinearPb2DJacobi::sendInfo >(loopMsg);
        }

    }

};


int main(int argc, char** argv) {

    size_t numX_objs = default_num_objs;
    size_t numY_objs = default_num_objs;

    size_t maxIter = 10;

    std::string name(argv[0]);

    if (argc == 1) {
        ::fmt::print(
                stderr, "{}: using default arguments since none provided\n", name
        );
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
        else {
            ::fmt::print(stderr,
                         "usage: {} <num-objects> <num-rows-per-object> <maxiter>\n",
                         name);
            return 1;
        }
    }
    CollectiveOps::initialize(argc, argv);

    auto const& this_node = theContext()->getNode();
    auto const& num_nodes = theContext()->getNumNodes();

    if (this_node == 0) {

        using BaseIndexType = typename Index2D::DenseIndexType;
        auto const& range = Index2D(static_cast<BaseIndexType>(numX_objs),
                                    static_cast<BaseIndexType>(numY_objs));

        auto proxy = vt::theCollection()->construct<LinearPb2DJacobi>(range);
        auto rootMsg = makeSharedMessage< LinearPb2DJacobi::LPMsg >
                                   (numX_objs, numY_objs, maxIter);
        proxy.broadcast<LinearPb2DJacobi::LPMsg,&LinearPb2DJacobi::solve>(rootMsg);

    }

    while (!rt->isTerminated()) {
        runScheduler();
    }

    CollectiveOps::finalize();

    return 0;

}

