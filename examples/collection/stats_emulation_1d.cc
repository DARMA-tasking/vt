/*
//@HEADER
// *****************************************************************************
//
//                           stats_emulation_1d.cc
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
#include <vt/vrt/collection/balance/load_stats_replayer.h>
#include <vt/vrt/collection/balance/load_stats_replayer.impl.h>

#include <cinttypes>

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vtAbortIf(
    argc != 5,
    "Must have four arguments: <num elms>, <initial phase>, "
    "<num phases to run>, <num phases to simulate>"
  );

  // number of collection elements
  int32_t num_elms = atoi(argv[1]);
  // initial phase to import from the stats files
  int initial_phase = atoi(argv[2]);
  // phases to run after loading object stats
  int32_t phases_to_run = atoi(argv[3]);
  // phases to simulate before switching to emulation
  int32_t phases_to_simulate = atoi(argv[4]);

  auto proxy = vt::theLoadStatsReplayer()->create1DAndConfigureEmulation(
    num_elms, initial_phase, phases_to_run, phases_to_simulate
  );

  auto const node = vt::theContext()->getNode();

  using clock = std::chrono::high_resolution_clock;
  clock::time_point phstart = clock::now();
  clock::time_point lbstart = phstart;

  if (node == 0)
    vt_print(replay, "Timestepping...\n");

  for (int i = 0; i < phases_to_run; i++) {
    if (node == 0) {
      if (i < phases_to_simulate) {
        vt_print(replay, "Simulating phase {}...\n", i + initial_phase);
      } else {
        vt_print(replay, "Emulating phase {}...\n", i + initial_phase);
      }
    }

    vt::runInEpochCollective([&proxy, i]{
      vt::theLoadStatsReplayer()->emulatePhase(proxy, i);
    });

    clock::time_point finished = clock::now();
    double phase_elapsed = std::chrono::duration_cast<
      std::chrono::duration<double>
    >(finished - phstart).count();
    double mig_plus_phase_elapsed = std::chrono::duration_cast<
      std::chrono::duration<double>
    >(finished - lbstart).count();

    if (node == 0) {
      vt_print(
        replay,
        "Phase {} time: mig_plus_phase={} sec, phase_only={} sec\n",
        i, mig_plus_phase_elapsed, phase_elapsed
      );
    }

    lbstart = clock::now();
    vt::thePhase()->nextPhaseCollective();
    phstart = clock::now();
  }

  vt::finalize();

  return 0;
}
