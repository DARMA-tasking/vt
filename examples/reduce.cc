

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


#include "vt/transport.h"
#include <cmath>
#include <cstdlib>


using namespace ::vt;


static constexpr std::size_t const default_nparts_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr std::size_t const verbose = 1;

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


struct Integration1D : vt::Collection<Integration1D,Index1D> {

private:

  size_t numObjs_ = default_num_objs;
  size_t numPartsPerObject_ = default_nparts_object;
  double quadValue = 0.0;

public:

  explicit Integration1D()
    : vt::Collection<Integration1D, Index1D>(),
      numObjs_(default_num_objs), numPartsPerObject_(default_nparts_object),
      quadValue(0.0)
  { }

  //
  // QuadSum: structure used for the reduction without callback
  //
  struct QuadSum {

    void operator()(vt::collective::ReduceTMsg<double>* msg) {
      //
      // The root index knows the reduced value.
      // It is obtained with ' msg->getConstVal() '
      //
      std::cout << " ## The integral over [0, 1] is "
                << msg->getConstVal()
                << "\n";
      std::cout << " ## The absolute error is "
                << abs(msg->getConstVal() - exactIntegral)
                << "\n";
      //
      // Set the 'root_reduce_finished' variable to true.
      //
      root_reduce_finished = true;
    }

  };

  struct CheckIntegral {

    void operator()(vt::collective::ReduceTMsg<double>* msg) {
      std::cout << " >> The numerical value for the integral over [0, 1] is "
                << msg->getConstVal()
                << "\n";
      std::cout << " >> The absolute error is "
                << abs(msg->getConstVal() - exactIntegral)
                << "\n";
      //
      // Set the 'root_reduce_finished' variable to true.
      //
      root_reduce_finished = true;
    }

  };

  struct InitMsg : vt::CollectionMessage<Integration1D> {

    size_t numObjects = 0;
    size_t nIntervalPerObject = 0;

    InitMsg() = default;

    InitMsg(const size_t nobjs, const size_t nint) :
      CollectionMessage<Integration1D>(),
      numObjects(nobjs), nIntervalPerObject(nint)
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
      std::cout << " Interval [" << a << ", " << b << "] "
                << ", on node " << theContext()->getNode()
                << " & object " << getIndex().x()
                << ", has integral " << quadsum
                << "\n";
    }

    //
    // Reduce the partial sum to get the integral over [0, 1]
    //

    auto proxy = this->getCollectionProxy();
    auto msgCB = vt::makeSharedMessage<
      vt::collective::ReduceTMsg<double>
      >(quadsum);
    auto cback = vt::theCB()->makeSend<CheckIntegral>(0);
    proxy.reduce<vt::collective::PlusOp<double>>(msgCB,cback);

    //
    // Syntax without a callback function:
    //
//    auto proxy = this->getCollectionProxy();
//    auto msg2 = vt::makeSharedMessage<
//      vt::collective::ReduceTMsg<double>
//      >(quadsum);
//    proxy.reduce<collective::PlusOp<double>, QuadSum>(msg2);

  }

};


int main(int argc, char** argv) {

  size_t num_objs = default_num_objs;
  size_t numIntPerObject = default_nparts_object;

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
      numIntPerObject = (size_t) strtol(argv[2], nullptr, 10);
    }
    else {
      ::fmt::print(stderr,
                   "usage: {} <num-objects> <num-interval-per-object>\n",
                   name);
      return 1;
    }
  }

  vt::CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (this_node == 0) {
    //
    // Create the interval decomposition into objects
    //
    using BaseIndexType = typename Index1D::DenseIndexType;
    auto const& range = Index1D(static_cast<BaseIndexType>(num_objs));

    auto proxy = vt::theCollection()->construct<Integration1D>(range);
    auto rootMsg = makeSharedMessage< Integration1D::InitMsg >
      (num_objs, numIntPerObject);
    proxy.broadcast<Integration1D::InitMsg,&Integration1D::compute>(rootMsg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  // Add something like this to validate the reduction.
  // Create the variable root_reduce_finished as a static variable,
  // which is only checked on one node.
  theTerm()->addAction([]{
    vtAssertExpr(root_reduce_finished == true);
  });

  vt::CollectiveOps::finalize();

  return 0;

}

