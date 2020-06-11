/*
//@HEADER
// *****************************************************************************
//
//                                   param.cc
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

#include <vt/transport.h>

#define VT_COMPILE_PARAM_EXAMPLE 0

#if VT_COMPILE_PARAM_EXAMPLE

static void fnTest(int a, int b, bool x) {
  fmt::print("fn: a={}, b={}, x={}\n", a, b, x ? "true" : "false");
}

static void fnTest2(int x, int y) {
  fmt::print("fn2: x={},y={}\n",x,y);
}

static void fnTest3(int x, double y) {
  fmt::print("fn3: x={},y={}\n",x,y);
}

struct FunctorTest1 {
  void operator()(int x, double y) const {
    fmt::print("FunctorTest1: x={},y={}\n",x,y);
  }
};

#endif

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

#if VT_COMPILE_PARAM_EXAMPLE
  vt::NodeType this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    vt::theParam()->sendData(1, vt::buildData(10, 20, false), PARAM_FUNCTION_RHS(fnTest));
    vt::theParam()->sendData(1, PARAM_FUNCTION_RHS(fnTest), 50, 29, false);
    vt::theParam()->sendData<PARAM_FUNCTION(fnTest)>(1, vt::buildData(10, 20, false));
    vt::theParam()->sendData<PARAM_FUNCTION(fnTest)>(1, 45, 23, true);

    vt::theParam()->sendData<PARAM_FUNCTION(fnTest2)>(1, 20, 10);
    vt::theParam()->sendData<PARAM_FUNCTION(fnTest3)>(1, 20, 50.0);

    vt::theParam()->sendData<FunctorTest1>(1, vt::buildData(20, 50.0));
    vt::theParam()->sendData<FunctorTest1>(1, 20, 100.0);
    vt::theParam()->sendData<FunctorTest1>(1, vt::buildData(10, 70.0));
  }
#endif

  vt::finalize();

  return 0;
}
