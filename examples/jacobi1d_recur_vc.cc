
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;

static constexpr int64_t const total_size = 1024;
static constexpr int64_t const block_size = 64;

struct CreateJacobi1DMsg : vt::vrt::VrtContextMessage {
  VrtContext_ProxyType parent;
  int64_t lo = 0, hi = 0;

  CreateJacobi1DMsg(
    int64_t const& in_lo, int64_t const& in_hi, VrtContext_ProxyType in_parent
  ) : VrtContextMessage(), parent(in_parent), lo(in_lo), hi(in_hi)
  { }
};

struct Jacobi1D;
static void create_jacobi1d(CreateJacobi1DMsg* msg, Jacobi1D* j1d);

struct Jacobi1D : vt::vrt::VrtContext {
  bool has_parent = false;
  VrtContext_ProxyType parent;
  VrtContext_ProxyType c1, c2;
  int64_t lo = 0, hi = 0;

  Jacobi1D(
    int64_t const& in_lo, int64_t const& in_hi, VrtContext_ProxyType in_parent
  ) : parent(in_parent), lo(in_lo), hi(in_hi)
  {
    printf("construct: lo=%lld, hi=%lld, parent=%lld\n", lo, hi, parent);
  }

  void create_children() {
    auto proxy = getProxy();
    auto const size = hi - lo;
    auto const mid = size / 2 + lo;

    printf(
      "create_children: lo=%lld, mid=%lld, hi=%lld, size=%lld\n",
      lo, mid, hi, size
    );

    c1 = theVrtCManager->constructVrtContext<Jacobi1D>(lo, mid, proxy);
    c2 = theVrtCManager->constructVrtContext<Jacobi1D>(mid, hi, proxy);

    {
      CreateJacobi1DMsg* msg = new CreateJacobi1DMsg(lo, mid, proxy);
      theVrtCManager->sendMsg<Jacobi1D, CreateJacobi1DMsg, create_jacobi1d>(
        c1, msg, [=]{ delete msg; }
      );
    }

    {
      CreateJacobi1DMsg* msg = new CreateJacobi1DMsg(mid, hi, proxy);
      theVrtCManager->sendMsg<Jacobi1D, CreateJacobi1DMsg, create_jacobi1d>(
        c2, msg, [=]{ delete msg; }
      );
    }
  }
};

static void create_jacobi1d(CreateJacobi1DMsg* msg, Jacobi1D* j1d) {
  auto const this_node = theContext->getNode();
  auto const lo = msg->lo;
  auto const hi = msg->hi;
  auto const size = hi - lo;
  auto const mid = size / 2 + lo;

  printf(
    "%d: lo=%lld, mid=%lld, hi=%lld, size=%lld\n", this_node, lo, mid, hi, size
  );

  if (size > block_size) {
    j1d->create_children();
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (my_node == 0) {
    auto root = theVrtCManager->constructVrtContext<Jacobi1D>(0, total_size, -1);

    //CreateJacobi1DMsg* msg = new CreateJacobi1DMsg(0, total_size, root);
    theVrtCManager->sendMsg<Jacobi1D, CreateJacobi1DMsg, create_jacobi1d>(
      root, makeSharedMessage<CreateJacobi1DMsg>(0, total_size, root)
    );
  }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
