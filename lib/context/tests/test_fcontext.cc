/*
//@HEADER
// *****************************************************************************
//
//                               test_fcontext.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include <time.h>
#include <cstdio>

#include "context/fcontext.h"

fcontext_t ctx;
fcontext_t ctx2;

inline void sleep(uint32_t _ms) {
  timespec req = { (time_t)_ms / 1000, (long)((_ms % 1000) * 1000000) };
  timespec rem = { 0, 0 };
  nanosleep(&req, &rem);
}

static void bar(fcontext_transfer_t t) {
  puts("BAR");
  sleep(1000);
  jump_fcontext(t.ctx, NULL);
}

static void foo(fcontext_transfer_t t) {
  puts("FOO");
  sleep(1000);
  jump_fcontext(ctx2, NULL);
  puts("FOO 2");
  sleep(1000);
  jump_fcontext(t.ctx, NULL);
}

int main(int argc, char** argv) {
  fcontext_stack_t s = create_fcontext_stack(16 * 1024);
  fcontext_stack_t s2 = create_fcontext_stack();

  ctx = make_fcontext(s.sptr, s.ssize, foo);
  ctx2 = make_fcontext(s2.sptr, s2.ssize, bar);

  jump_fcontext(ctx, NULL);
  puts("END");

  destroy_fcontext_stack(&s);
  destroy_fcontext_stack(&s2);
  return 0;
}
