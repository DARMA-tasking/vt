/*
//@HEADER
// *****************************************************************************
//
//                                jump_test_1.cc
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

#include "context/context_wrapper.h"
#include "context/fcontext.h"
#include "context/context_stack.h"
#include "util.h"

#include <time.h>
#include <cstdio>

using namespace fcontext;
using namespace fcontext::examples;

static Context ctx1;
static Context ctx2;

static void fn1(fcontext_transfer_t t) {
  puts("fn1 1");
  sleep(100);
  jumpContext(t);
  puts("fn1 2");
  sleep(100);
  jumpContext(t);
}

static void fn2(fcontext_transfer_t t) {
  puts("fn2 1");
  sleep(100);
  jumpContext(t);
  puts("fn2 2");
  sleep(100);
  jumpContext(t);
}

int main(int argc, char** argv) {
  ULTContextType s1 = createStack();
  ULTContextType s2 = createStack();

  ctx1 = makeContext(s1, fn1);
  ctx2 = makeContext(s2, fn2);

  puts("main 1");
  auto t1 = jumpContext(ctx1);
  puts("main 2");
  auto t2 = jumpContext(ctx2);
  puts("main 3");
  jumpContext(t1.transfer);
  puts("main 4");
  jumpContext(t2.transfer);
  puts("END");

  destroyStack(s1);
  destroyStack(s2);

  return 0;
}
