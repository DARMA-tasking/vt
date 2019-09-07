/*
//@HEADER
// *****************************************************************************
//
//                             jacobi1d_recur_vc.cc
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

#include "vt/transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;

static constexpr int64_t const total_size = 1024;
static constexpr int64_t const block_size = 64;

struct CreateJacobi1DMsg : vt::vrt::VirtualMessage {
  VirtualProxyType parent;
  int64_t lo = 0, hi = 0;

  CreateJacobi1DMsg(
    int64_t const& in_lo, int64_t const& in_hi, VirtualProxyType in_parent
  ) : VirtualMessage(), parent(in_parent), lo(in_lo), hi(in_hi)
  { }
};

struct Jacobi1D;
static void create_jacobi1d(CreateJacobi1DMsg* msg, Jacobi1D* j1d);

struct Jacobi1D : vt::vrt::VirtualContext {
  bool has_parent = false;
  VirtualProxyType parent;
  VirtualProxyType c1, c2;
  int64_t lo = 0, hi = 0;

  Jacobi1D(
    int64_t const& in_lo, int64_t const& in_hi, VirtualProxyType in_parent
  ) : parent(in_parent), lo(in_lo), hi(in_hi)
  {
    fmt::print("construct: lo={}, hi={}, parent={}\n", lo, hi, parent);
  }

  void create_children() {
    auto proxy = getProxy();
    auto const size = hi - lo;
    auto const mid = size / 2 + lo;

    fmt::print(
      "create_children: lo={}, mid={}, hi={}, size={}\n",
      lo, mid, hi, size
    );

    c1 = theVirtualManager()->makeVirtual<Jacobi1D>(lo, mid, proxy);
    c2 = theVirtualManager()->makeVirtual<Jacobi1D>(mid, hi, proxy);

    {
      auto msg = makeSharedMessage<CreateJacobi1DMsg>(lo, mid, proxy);
      theVirtualManager()->sendMsg<Jacobi1D, CreateJacobi1DMsg, create_jacobi1d>(
        c1, msg
      );
    }

    {
      auto msg = makeSharedMessage<CreateJacobi1DMsg>(mid, hi, proxy);
      theVirtualManager()->sendMsg<Jacobi1D, CreateJacobi1DMsg, create_jacobi1d>(
        c2, msg
      );
    }
  }
};

static void create_jacobi1d(CreateJacobi1DMsg* msg, Jacobi1D* j1d) {
  auto const this_node = theContext()->getNode();
  auto const lo = msg->lo;
  auto const hi = msg->hi;
  auto const size = hi - lo;
  auto const mid = size / 2 + lo;

  fmt::print(
    "{}: lo={}, mid={}, hi={}, size={}\n", this_node, lo, mid, hi, size
  );

  if (size > block_size) {
    j1d->create_children();
  }
}

#define sstmac_app_name jacobi1d_recur_vc_vt

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    auto root = theVirtualManager()->makeVirtual<Jacobi1D>(0, total_size, -1);

    //CreateJacobi1DMsg* msg = new CreateJacobi1DMsg(0, total_size, root);
    theVirtualManager()->sendMsg<Jacobi1D, CreateJacobi1DMsg, create_jacobi1d>(
      root, makeSharedMessage<CreateJacobi1DMsg>(0, total_size, root)
    );
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
