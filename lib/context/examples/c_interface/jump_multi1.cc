/*
//@HEADER
// ************************************************************************
//
//                          jump_multi1.cc
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

#include "fcontext.h"
#include "util.h"

#include <cstdio>

using namespace fcontext::examples;

static fcontext_t ctx1;
static fcontext_t ctx2;

static void fn1(fcontext_transfer_t t) {
  puts("fn1 1");
  sleep(100);
  auto t1 = jump_fcontext(t.ctx, nullptr);
  puts("fn1 2");
  sleep(100);
  jump_fcontext(t1.ctx, nullptr);
}

static void fn2(fcontext_transfer_t t) {
  puts("fn2 1");
  sleep(100);
  auto t1 = jump_fcontext(t.ctx, nullptr);
  puts("fn2 2");
  sleep(100);
  jump_fcontext(t1.ctx, nullptr);
}

int main(int argc, char** argv) {
  fcontext_stack_t s1 = create_fcontext_stack();
  fcontext_stack_t s2 = create_fcontext_stack();

  ctx1 = make_fcontext_stack(s1, fn1);
  ctx2 = make_fcontext_stack(s2, fn2);

  puts("main 1");
  auto t1 = jump_fcontext(ctx1, nullptr);
  puts("main 2");
  auto t2 = jump_fcontext(ctx2, nullptr);
  puts("main 3");
  auto t3 = jump_fcontext(t1.ctx, nullptr);
  puts("main 4");
  auto t4 = jump_fcontext(t2.ctx, nullptr);
  puts("END");

  destroy_fcontext_stack(s1);
  destroy_fcontext_stack(s2);

  return 0;
}
