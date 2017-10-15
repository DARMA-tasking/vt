
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;

struct TestMsg : vt::vrt::VirtualMessage {
  int from = 0;

  TestMsg(int const& in_from)
    : VirtualMessage(), from(in_from)
  { }
};

struct MainVC : vt::vrt::MainVirtualContext {
  int my_data = -1;

  MainVC() : my_data(29) {
    printf("constructing MainVC: data=%d\n", my_data);
  }
};

static void testHan(TestMsg* msg, MainVC* vc) {
  printf("testHan: msg->from=%d, my_data=%d\n", msg->from, vc->my_data);
}

VT_REGISTER_MAIN_CONTEXT(MainVC);
