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

#include "vt/transport.h"

#include <cstdlib>
#include <cassert>
#include <iostream>


//
// This code applies a few steps of the Jacobi iteration to
// the linear system  A x = 0
// where is the tridiagonal matrix with pattern [-1 2 -1]
// The initial guess for x is random.
// The exact solution is the vector 0.
//
// The matrix A is square and invertible.
// The number of rows is ((number of objects) * (number of rows per object))
//

using namespace ::vt;


static constexpr std::size_t const default_nrow_object = 8;
static constexpr std::size_t const default_num_objs = 4;


struct LinearPb1DJacobi : vt::Collection<LinearPb1DJacobi,Index1D> {

private:

    Index1D idx_;
    std::vector<double> tcur_, told_;
    std::vector<double> rhs_;
    size_t iter_ = 0, objDone_ = 0;
    size_t msgReceived_ = 0, totalReceive_ = 0;
    size_t numObjs_ = 1;
    size_t numRowsPerObject_ = 1;
    size_t maxIter_ = 5;

public:

    LinearPb1DJacobi() = default;

    LinearPb1DJacobi(Index1D in_idx)
            : vt::Collection<LinearPb1DJacobi,Index1D>(), idx_(in_idx),
            tcur_(), told_(), rhs_(), iter_(0), objDone_(0),
            msgReceived_(0), totalReceive_(0),
            numObjs_(1), numRowsPerObject_(1), maxIter_(5)
    { }


    struct BlankMsg : vt::CollectionMessage<LinearPb1DJacobi> {
        BlankMsg() = default;
    };


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


    struct VecMsg : vt::CollectionMessage<LinearPb1DJacobi> {

        IndexType from_index;
        double val = 0.0;

        VecMsg() = default;
        VecMsg(IndexType const& in_index, double const& ref) :
                vt::CollectionMessage<LinearPb1DJacobi>(),
                from_index(in_index), val(ref)
        { }

    };


    struct NormReduceMsg : vt::collective::ReduceMsg {
        double norm = 0.0;
        NormReduceMsg() = default;
        NormReduceMsg(double const ref) : norm(ref)
        { }
    };


    static void reduceNorm(NormReduceMsg* msg) {

        if (msg->isRoot()) {
            fmt::print(" Norm >> at root {}: final {} \n",
                    theContext()->getNode(), msg->norm);
        } else {
            NormReduceMsg* fst_msg = msg;
            auto cur_msg = msg->getNext<NormReduceMsg>();
            while (cur_msg != nullptr) {
                fst_msg->norm = (fst_msg->norm > cur_msg->norm) ?
                                fst_msg->norm : cur_msg->norm;
                cur_msg = cur_msg->getNext<NormReduceMsg>();
            }
        }

    }

    void checkCompletion() {

        if ((iter_ < maxIter_) && (idx_.x() == 0)) {
            auto proxy = this->getCollectionProxy();
            auto loopMsg = makeSharedMessage< LinearPb1DJacobi::BlankMsg >();
            proxy.broadcast< LinearPb1DJacobi::BlankMsg,
                            &LinearPb1DJacobi::sendInfo>(loopMsg);
        }

    }


    struct IterMsg : CollectionMessage<LinearPb1DJacobi> {
        size_t iterID = 0;
        IterMsg() = default;
        IterMsg(size_t imax) : CollectionMessage<LinearPb1DJacobi>(),
                               iterID(imax)
        { }
    };


    void getNorm(BlankMsg *msg) {

        double maxNorm = 0.0;
        for (auto val : tcur_) {
            maxNorm = (maxNorm > abs(val)) ? maxNorm : abs(val);
        }

        auto reduce_msg = makeSharedMessage<NormReduceMsg>(maxNorm);
        auto proxy = this->getCollectionProxy();
        theCollection()->reduceMsg< LinearPb1DJacobi,
                LinearPb1DJacobi::NormReduceMsg,
                LinearPb1DJacobi::reduceNorm >(proxy, reduce_msg);

        checkCompletion();

    }


    void countObjects(IterMsg *msg) {

        if (idx_.x())
            return;

        if ((iter_ <= msg->iterID) and (msg->iterID <= iter_+1))
            objDone_ += 1;

        if (objDone_ == numObjs_) {
            objDone_ = 0;
            auto proxy = this->getCollectionProxy();
            auto normMsg = makeSharedMessage< LinearPb1DJacobi::BlankMsg >();
            proxy.broadcast< LinearPb1DJacobi::BlankMsg,
                           &LinearPb1DJacobi::getNorm >(normMsg);
        }

    }


    void doIteration() {

        //--- Copy extremal values
        tcur_[0] = told_[0];
        tcur_[numRowsPerObject_+1] = told_[numRowsPerObject_+1];

        //
        //---- Jacobi iteration step for
        //---- A tridiagonal matrix = "tridiag" ( [-1.0  2.0  -1.0] )
        //---- rhs_ right hand side vector
        //
        for (size_t ii = 1; ii <= numRowsPerObject_; ++ii) {
            tcur_[ii] = 0.5*(rhs_[ii] + told_[ii-1] + told_[ii+1]);
        }

        iter_ += 1;

        auto proxy = this->getCollectionProxy();
        auto doneMsg = vt::makeSharedMessage< IterMsg >(iter_);
        theCollection()->sendMsg< LinearPb1DJacobi::IterMsg,
                &LinearPb1DJacobi::countObjects > (proxy(0), doneMsg);

        memcpy(&told_[0], &tcur_[0], sizeof(double)*tcur_.size());

    }


    void exchange(VecMsg *msg) {

        // Receive and treat the message from a neighboring object.

        if (this->idx_.x() > msg->from_index.x()) {
            this->told_[0] = msg->val;
            msgReceived_ += 1;
        }

        if (this->idx_.x() < msg->from_index.x()) {
            this->told_[numRowsPerObject_ + 1] = msg->val;
            msgReceived_ += 1;
        }

        // Check whether this 'object' has received all the expected messages.
        if (msgReceived_ == totalReceive_) {
            msgReceived_ = 0;
            doIteration();
        }

    }


    void sendInfo(BlankMsg *msg) {

        // Routine to send information to a different object

        if (numObjs_ == 1) {
            doIteration();
            return;
        }

        //--- Send the values to the left
        auto proxy = this->getCollectionProxy();
        if (idx_.x() > 0) {
            auto leftMsg = vt::makeSharedMessage< VecMsg >(idx_, told_[1]);
            theCollection()->sendMsg< LinearPb1DJacobi::VecMsg,
                    &LinearPb1DJacobi::exchange > (proxy(idx_.x()-1), leftMsg);
        }

        //--- Send values to the right
        if (idx_.x() < numObjs_ - 1) {
            auto rightMsg = vt::makeSharedMessage< VecMsg >(idx_,
                                   told_[numRowsPerObject_]);
            theCollection()->sendMsg< LinearPb1DJacobi::VecMsg,
                    &LinearPb1DJacobi::exchange > (proxy(idx_.x()+1), rightMsg);
        }

    }


    void init() {

        tcur_.assign(numRowsPerObject_ + 2, 0.0);
        told_.assign(numRowsPerObject_ + 2, 0.0);
        rhs_.assign(numRowsPerObject_ + 2, 0.0);

        for (size_t ii = 0; ii < tcur_.size(); ++ii) {
            tcur_[ii] = std::rand() / (double(RAND_MAX) + 1.0);
        }

        totalReceive_ = 2;

        if (idx_.x() == 0) {
            tcur_[0] = 0.0;
            totalReceive_ -= 1;
        }

        if (idx_.x() == numObjs_ - 1) {
            tcur_[numRowsPerObject_+1] = 0.0;
            totalReceive_ -= 1;
        }

        memcpy(&told_[0], &tcur_[0], sizeof(double)*tcur_.size());

    }


    void solve(LPMsg* msg) {

        numObjs_ = msg->numObjects;
        numRowsPerObject_ = msg->nRowPerObject;
        maxIter_ = msg->iterMax;

        // Initialize the starting vector
        init();

        // Ask all objects to run through the 'sendInfo' routine.
        if (idx_.x() == 0) {
            auto proxy = this->getCollectionProxy();
            auto loopMsg = makeSharedMessage< LinearPb1DJacobi::BlankMsg >();
            proxy.broadcast< LinearPb1DJacobi::BlankMsg,
                            &LinearPb1DJacobi::sendInfo >(loopMsg);
        }

    }

};


int main(int argc, char** argv) {

    CollectiveOps::initialize(argc, argv);

    auto const& this_node = theContext()->getNode();
    auto const& num_nodes = theContext()->getNumNodes();

    size_t num_objs = default_num_objs;
    size_t numRowsPerObject = default_nrow_object;
    size_t maxIter = 5;

    std::string name(argv[0]);
    if (argc == 1) {
        ::fmt::print(
                stderr, "{}: using default arguments since none provided\n", name
        );
    } else {
        if (argc == 2) {
            num_objs = (size_t) strtol(argv[1], nullptr, 10);
        }
        else if (argc == 3) {
            num_objs = (size_t) strtol(argv[1], nullptr, 10);
            numRowsPerObject = (size_t) strtol(argv[2], nullptr, 10);
        }
        else if (argc == 4) {
            num_objs = (size_t) strtol(argv[1], nullptr, 10);
            numRowsPerObject = (size_t) strtol(argv[2], nullptr, 10);
            maxIter = (size_t) strtol(argv[3], nullptr, 10);
        }
        else {
            std::string const buf = fmt::format(
                    "usage: {} <num-objects> <num-rows-per-object> <maxiter>",
                    name);
            if (this_node == 0) {
                CollectiveOps::output(buf);
                CollectiveOps::finalize();
                return 1;
            }
            CollectiveOps::finalize();
            return 1;
        }
    }

    if (this_node == 0) {

        // Create the decomposition into objects
        using BaseIndexType = typename Index1D::DenseIndexType;
        auto const& range = Index1D(static_cast<BaseIndexType>(num_objs));

        auto proxy = vt::theCollection()->construct<LinearPb1DJacobi>(range);
        auto rootMsg = makeSharedMessage< LinearPb1DJacobi::LPMsg >
                                      (num_objs, numRowsPerObject, maxIter);
        proxy.broadcast<LinearPb1DJacobi::LPMsg,&LinearPb1DJacobi::solve>(rootMsg);

    }

    while (!rt->isTerminated()) {
        runScheduler();
    }

    CollectiveOps::finalize();

    return 0;

}

