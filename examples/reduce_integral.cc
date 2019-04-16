
#include "vt/transport.h"
#include <cmath>
#include <cstdlib>


using namespace ::vt;


static constexpr std::size_t const default_nint_object = 8;
static constexpr std::size_t const default_num_objs = 4;
static constexpr std::size_t const verbose = 1;


//
// This code computes a composite trapezoidal rule for the integral
// \int_{0}^{1} f(x) dx
// The function 'f' is defined in the code.
// The interval [0, 1] is broken into (# of objects) * (# of sub-interval per object)
//


//
// Function to integrate over [0, 1]
//

double f(double x) {
//    return x*x;
  return sin(M_PI * x);
}


struct Integration1D : vt::Collection<Integration1D,Index1D> {

private:

    Index1D idx_;
    size_t numObjs_;
    size_t numIntPerObject_;
    double quadValue;

public:

    Integration1D() = default;

    explicit Integration1D(Index1D in_idx)
            : vt::Collection<Integration1D, Index1D>(), idx_(in_idx),
              numObjs_(1), numIntPerObject_(1),
              quadValue(0.0) {}


    void setGlobalIntegral(const double val) { quadValue = val; };


    struct BlankMsg : vt::CollectionMessage<Integration1D> {
        BlankMsg() = default;
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


    struct QuadMsg : vt::collective::ReduceTMsg<double> {

        Integration1D *integral_;

        explicit QuadMsg(double in_val, Integration1D *pbref)
                : ReduceTMsg<double>(in_val), integral_(pbref)
        { }

    };


    struct QuadSum {
        void operator()(QuadMsg* msg) {
            if (msg->isRoot()) {
                //
                // The root index knows the reduced value.
                // It is obtained with ' msg->getConstVal() '
                //
                std::cout << " >> The integral over [0, 1] is "
                            << msg->getConstVal() << "\n";
                //
                // Send the result back to the calling structure
                //
                msg->integral_->setGlobalIntegral(msg->getConstVal());
            }
        }
    };


    void compute(InitMsg *msg) {

        numObjs_ = msg->numObjects;
        numIntPerObject_ = msg->nIntervalPerObject;

        //
        // Compute the integral with the trapezoidal rule
        // over the interval
        // [numIntPerObject_*idx_.x()*h, numIntPerObject_*(idx_.x() + 1)*h]
        //

        double h = 1.0 / (numIntPerObject_ * numObjs_ );
        double quadsum = 0.0;

        for (size_t ii = 0; ii < numIntPerObject_; ++ii) {
            double x0 = ( numIntPerObject_ * idx_.x() + ii) * h;
            /* --- Trapeze Quadrature Rule over the interval [x0, x0+h] */
            quadsum += 0.5 * h * ( f(x0) + f(x0+h) );
        }

        if (verbose > 0) {
            double a =  numIntPerObject_ * idx_.x() * h;
            double b = a + numIntPerObject_ * h;

            std::cout << " Interval [" << a << ", " << b << "] "
                        << ", on node " << theContext()->getNode()
                        << " & object " << idx_.x()
                        << ", has integral " << quadsum
                        << "\n";
        }

        //
        // Reduce the partial sum to get the integral over [0, 1]
        //

        auto proxy = this->getCollectionProxy();
        auto msg2 = makeSharedMessage<QuadMsg>(quadsum, this);
        theCollection()->reduceMsg<
                Integration1D,
                QuadMsg,
                QuadMsg::template msgHandler<
                        QuadMsg, collective::PlusOp<double>, QuadSum >
        >(proxy, msg2);

    }

};


int main(int argc, char** argv) {

    size_t num_objs = default_num_objs;
    size_t numIntPerObject = default_nint_object;

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

        // Create the decomposition into objects
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

    vt::CollectiveOps::finalize();

    return 0;
}

