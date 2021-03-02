/*
//@HEADER
// *****************************************************************************
//
//                              memory_checker.cc
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
#include <vt/utils/memory/memory_usage.h>

std::vector<int32_t*> ptrs;

void allocateAndTouch(std::size_t num_bytes) {
  auto ptr = new int32_t[num_bytes / 4];
  for (std::size_t i = 0; i < num_bytes / 4; i++) {
    ptr[i] = 20 * i + 23;
  }
  ptrs.push_back(ptr);
}

void deallocate() {
  if (ptrs.size() > 0) {
    auto ptr = ptrs.back();
    ptrs.pop_back();
    delete [] ptr;
  }
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  auto this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    auto usage = vt::theMemUsage();
    fmt::print("Initial: {}\n", usage->getUsageAll());

    for (int i = 0; i < 32; i++) {
      allocateAndTouch(1024 * 1024 * 64);
      fmt::print("After alloc {} MiB: {}\n", (i + 1) * 64, usage->getUsageAll());
      vt::runScheduler();
    }

    for (int i = 0; i < 32; i++) {
      deallocate();
      fmt::print("After de-alloc {} MiB: {}\n", (32 * 64) - (i + 1) * 64, usage->getUsageAll());
      vt::runScheduler();
    }
  }

  vt::theSched()->runSchedulerWhile([]{ return !vt::rt->isTerminated(); });

  vt::finalize();

  return 0;
}
