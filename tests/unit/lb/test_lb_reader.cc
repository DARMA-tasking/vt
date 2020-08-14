/*
//@HEADER
// *****************************************************************************
//
//                              test_lb_reader.cc
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
#include <vt/vrt/collection/balance/read_lb.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestLBReader = TestParallelHarness;

TEST_F(TestLBReader, test_lb_read_1) {

  std::string file_name = "test_lb_read_1.txt";
  std::ofstream out(file_name);
  out << ""
    "0 NoLB\n"
    "1 HierarchicalLB\n"
    "%10 HierarchicalLB\n";
  out.close();

  using Spec       = vt::vrt::collection::balance::ReadLBSpec;
  using SpecIdx    = vt::vrt::collection::balance::SpecIndex;
  using SpecLBType = vt::vrt::collection::balance::LBType;
  Spec::clear();
  EXPECT_EQ(Spec::openFile(file_name), true);
  Spec::readFile();

  EXPECT_EQ(Spec::numEntries(), 3);
  EXPECT_EQ(Spec::getExactEntries().size(), 2);
  EXPECT_EQ(Spec::getModEntries().size(), 1);

  for (SpecIdx i = 0; i < 21; i++) {
    auto entry = Spec::entry(i);
    switch (i) {
    case 0:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::NoLB);
      EXPECT_EQ(entry->getName(), "NoLB");
      break;
    case 1:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::HierarchicalLB);
      EXPECT_EQ(entry->getName(), "HierarchicalLB");
      break;
    case 10:
    case 20:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::HierarchicalLB);
      break;
    default:
      EXPECT_TRUE(entry == nullptr);
    }
  }

  theConfig()->colorize_output = false;
  std::string expected_spec =
    "vt: \tExact specification lines:\n"
    "vt: \tRun `NoLB` on phase 0\n"
    "vt: \tRun `HierarchicalLB` on phase 1\n"
    "vt: \tMod (%) specification lines:\n"
    "vt: \tRun `HierarchicalLB` every 10 phases\n";
  EXPECT_EQ(Spec::toString(), expected_spec);
}

TEST_F(TestLBReader, test_lb_read_2) {

  std::string file_name = "test_lb_read_2.txt";
  std::ofstream out(file_name);
  out << ""
    "0 NoLB\n"
    "1 HierarchicalLB min=0.9 max=1.1 auto=false\n"
    "%10 GossipLB c=1 k=5 f=2 i=10\n"
    "%5 GreedyLB min=1.0\n"
    "120 HierarchicalLB test_xyz=3\n";
  out.close();

  using Spec       = vt::vrt::collection::balance::ReadLBSpec;
  using SpecIdx    = vt::vrt::collection::balance::SpecIndex;
  using SpecLBType = vt::vrt::collection::balance::LBType;
  Spec::clear();
  Spec::openFile(file_name);
  Spec::readFile();

  EXPECT_EQ(Spec::numEntries(), 5);
  for (SpecIdx i = 0; i < 121; i++) {
    auto entry = Spec::entry(i);
    switch (i) {
    case 0:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::NoLB);
      EXPECT_TRUE(entry->getParams().empty());
      EXPECT_EQ(entry->getIdx(), 0);
      break;
    case 1:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::HierarchicalLB);
      EXPECT_TRUE(entry->getOrDefault<double>("min", 0.) == 0.9);
      EXPECT_TRUE(entry->getOrDefault<double>("max", 0.) == 1.1);
      EXPECT_TRUE(entry->getOrDefault<bool>("auto", true) == false);
      EXPECT_EQ(entry->getIdx(), 1);
      break;
    case 10:
    case 20:
    case 30:
    case 40:
    case 50:
    case 60:
    case 70:
    case 80:
    case 90:
    case 100:
    case 110:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::GossipLB);
      EXPECT_TRUE(entry->getOrDefault<int32_t>("c", 0) == 1);
      EXPECT_TRUE(entry->getOrDefault<int32_t>("k", 0) == 5);
      EXPECT_TRUE(entry->getOrDefault<int32_t>("f", 0) == 2);
      EXPECT_TRUE(entry->getOrDefault<int32_t>("i", 0) == 10);
      EXPECT_EQ(entry->getIdx(), 10);
      break;
    case 5:
    case 15:
    case 25:
    case 35:
    case 45:
    case 55:
    case 65:
    case 75:
    case 85:
    case 95:
    case 105:
    case 115:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::GreedyLB);
      EXPECT_TRUE(entry->getOrDefault<double>("min", 0.) == 1.0);
      EXPECT_EQ(entry->getParams().at("min"), "1.0");
      break;
    case 120:
      EXPECT_TRUE(entry != nullptr);
      EXPECT_TRUE(entry->getLB() == SpecLBType::HierarchicalLB);
      EXPECT_TRUE(entry->getOrDefault<int32_t>("test_xyz", 0) == 3);
      break;
    default:
      EXPECT_TRUE(entry == nullptr);
    }
  }

  theConfig()->colorize_output = false;
  std::string expected_spec =
    "vt: \tExact specification lines:\n"
    "vt: \tRun `NoLB` on phase 0\n"
    "vt: \tRun `HierarchicalLB` on phase 1 with arguments `auto=false max=1.1 min=0.9`\n"
    "vt: \tRun `HierarchicalLB` on phase 120 with arguments `test_xyz=3`\n"
    "vt: \tMod (%) specification lines:\n"
    "vt: \tRun `GreedyLB` every 5 phases with arguments `min=1.0` excluding phases 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120\n"
    "vt: \tRun `GossipLB` every 10 phases with arguments `c=1 f=2 i=10 k=5` excluding phases 120\n";
  EXPECT_EQ(Spec::toString(), expected_spec);
}

}}} // end namespace vt::tests::unit
