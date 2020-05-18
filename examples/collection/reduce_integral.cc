/*
//@HEADER
// *****************************************************************************
//
//                              reduce_integral.cc
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


//
// This code computes a composite trapezoidal rule for the integral
// \int_{0}^{1} f(x) dx
//
// The function 'f' is defined in the code.
//
// The interval [0, 1] is broken into uniform sub-intervals of length
//
// H = 1.0 / (# of objects)
//
// Each sub-interval [xi, xi + H] is broken into smaller parts of length
//
// h = H / (# of parts per object)
//


#include <vt/transport.h>
#include <cmath>

static constexpr std::size_t const default_nparts_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr std::size_t const verbose = 1;
static constexpr vt::NodeType const reduce_root_node = 0;

static bool root_reduce_finished = false;
static double exactIntegral = 0.0;


//
// Function 'f' to integrate over [0, 1]
//


double f(double x) {
  //----
  //exactIntegral = 1.0 / 3.0;
  //return x*x;
  //----
  exactIntegral = M_2_PI;
  return sin(M_PI * x);
}

using ReduceMsg = vt::collective::ReduceTMsg<double>;

struct Integration1D : vt::Collection<Integration1D, vt::Index1D> {

private:

  size_t numObjs_ = default_num_objs;
  size_t numPartsPerObject_ = default_nparts_object;

public:

  explicit Integration1D()
    : numObjs_(default_num_objs),
      numPartsPerObject_(default_nparts_object)
  { }

  struct CheckIntegral {

    void operator()(ReduceMsg* msg) {
      fmt::print(" >> The integral over [0, 1] is {}\n", msg->getConstVal());
      fmt::print(
        " >> The absolute error is {}\n",
        std::fabs(msg->getConstVal() - exactIntegral)
      );
      //
      // Set the 'root_reduce_finished' variable to true.
      //
      root_reduce_finished = true;
    }

  };

  struct InitMsg : vt::CollectionMessage<Integration1D> {

    size_t numObjects = 0;
    size_t nIntervalPerObject = 0;

    InitMsg(const size_t nobjs, const size_t nint)
      : numObjects(nobjs), nIntervalPerObject(nint)
    { }

  };

  void compute(InitMsg *msg) {

    numObjs_ = msg->numObjects;
    numPartsPerObject_ = msg->nIntervalPerObject;

    //
    // Compute the integral with the trapezoidal rule
    // over the interval [a, b]
    //
    // a = numPartsPerObject_ * (IndexID) * h
    //
    // b = a + numPartsPerObject_ * h
    //
    // where h = 1.0 / (numPartsPerObject_ * numObjs_ )
    //
    // Since 0 <= (IndexID) < numObjs_, the union of these intervals
    // covers [0, 1]
    //

    double h = 1.0 / (numPartsPerObject_ * numObjs_ );
    double quadsum = 0.0;

    double a = numPartsPerObject_ * getIndex().x() * h;

    //
    // Apply composite trapezoidal rule over
    //
    // [a, a + numPartsPerObject_ * h]
    //
    for (size_t ii = 0; ii < numPartsPerObject_; ++ii) {
      double x0 = a + ii * h;
      /* --- Trapeze Quadrature Rule over the interval [x0, x0+h] */
      quadsum += 0.5 * h * ( f(x0) + f(x0+h) );
    }

    if (verbose > 0) {
      double b = a + numPartsPerObject_ * h;
      fmt::print(
        " Interval [{}, {}], on node {} & object {}, "
        "has integral {}.\n", a, b, vt::theContext()->getNode(),
        getIndex(), quadsum
      );
    }

    //
    // Reduce the partial sum to get the integral over [0, 1]
    //

    auto proxy = this->getCollectionProxy();
    auto msgCB = vt::makeMessage<ReduceMsg>(quadsum);
    auto cback = vt::theCB()->makeSend<CheckIntegral>(reduce_root_node);
    proxy.reduce<vt::collective::PlusOp<double>>(msgCB.get(),cback);
  }

};


int main(int argc, char** argv) {

  size_t num_objs = default_num_objs;
  size_t numIntPerObject = default_nparts_object;

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
  } else {
    if (argc == 2) {
      num_objs = (size_t) strtol(argv[1], nullptr, 10);
    }
    else if (argc == 3) {
      num_objs = (size_t) strtol(argv[1], nullptr, 10);
      numIntPerObject = (size_t) strtol(argv[2], nullptr, 10);
    }
    else {
      fmt::print(
        stderr,
        "usage: {} <num-objects> <num-interval-per-object>\n", name
      );
      return 1;
    }
  }

  if (this_node == 0) {
    //
    // Create the interval decomposition into objects
    //
    using BaseIndexType = typename vt::Index1D::DenseIndexType;
    auto range = vt::Index1D(static_cast<BaseIndexType>(num_objs));

    auto proxy = vt::theCollection()->construct<Integration1D>(range);
    proxy.broadcast<Integration1D::InitMsg,&Integration1D::compute>(
      num_objs, numIntPerObject
    );
  }

  // Add something like this to validate the reduction.
  // Create the variable root_reduce_finished as a static variable,
  // which is only checked on one node.
  vt::theTerm()->addAction([]{
    if (vt::theContext()->getNode() == reduce_root_node) {
      vtAssertExpr(root_reduce_finished == true);
    }
  });

  vt::finalize();

  return 0;

}

