/*
//@HEADER
// *****************************************************************************
//
//                          polymorphic_collection.cc
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

#include <vt/transport.h>

/// [Polymorphic collection example]
static constexpr int32_t const default_num_elms = 16;
struct InitialConsTag{};

struct Hello : vt::Collection<Hello, vt::Index1D> {
  checkpoint_virtual_serialize_root()

  explicit Hello(InitialConsTag) {}
  explicit Hello(checkpoint::SERIALIZE_CONSTRUCT_TAG) {}

  virtual ~Hello() {}

  template <typename Serializer>
  void serialize(Serializer& s) {
    vt::Collection<Hello, vt::Index1D>::serialize(s);
    s | test_val;
  }

  virtual void doWork();

  double test_val = 0.0;
};

template <typename T>
struct HelloTyped : Hello {
  checkpoint_virtual_serialize_derived_from(Hello)

  explicit HelloTyped(InitialConsTag);
  explicit HelloTyped(checkpoint::SERIALIZE_CONSTRUCT_TAG)
    : Hello(checkpoint::SERIALIZE_CONSTRUCT_TAG{})
  {}

  virtual void doWork() override;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | my_vec;
  }

  std::vector<T> my_vec;
};

template <>
void HelloTyped<int>::doWork() {
  fmt::print("correctly doing this -- int!\n");
  Hello::doWork();
}

template <>
void HelloTyped<double>::doWork() {
  Hello::doWork();
  fmt::print("correctly doing this -- double!\n");
}

template <>
HelloTyped<int>::HelloTyped(InitialConsTag)
  : Hello(InitialConsTag{})
{
  for (int i = 0; i < 100; i++) {
    my_vec.push_back(i+200);
  }
}

template <>
HelloTyped<double>::HelloTyped(InitialConsTag)
  : Hello(InitialConsTag{})
{
  for (int i = 0; i < 100; i++) {
    my_vec.push_back(i + 0.5);
  }
}

void Hello::doWork() {
  vt_print(
    gen, "idx={}: val={}, type={}\n",
    getIndex(), test_val, typeid(*this).name()
  );

  auto hi = dynamic_cast<HelloTyped<int>*>(this);
  auto hd = dynamic_cast<HelloTyped<double>*>(this);

  if (hi) {
    for (int i = 0; i < 100; i++) {
      vtAssert(hi->my_vec[i] == i+200, "Must be equal");
    }
  } else if (hd) {
    for (int i = 0; i < 100; i++) {
      vtAssert(hd->my_vec[i] == i+0.5, "Must be equal");
    }
  } else {
    vtAbort("FAIL: wrong type of collection element\n");
  }
}


static void migrateToNext(Hello* col) {
  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();
  vt::NodeType next_node = (this_node + 1) % num_nodes;

  fmt::print("{}: migrateToNext: idx={}\n", this_node, col->getIndex());
  col->migrate(next_node);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  auto num_elms = std::max(static_cast<int32_t>(num_nodes), default_num_elms);

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  auto range = vt::Index1D(num_elms);

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<Hello>>> elms;
  vtAbortIf(num_elms % num_nodes != 0, "Must be even number of elements per rank");
  auto const num_per = num_elms / num_nodes;
  if (this_node % 2 == 0) {
    for (int i = this_node*num_per; i < (this_node+1)*num_per; i++) {
      vt::Index1D idx{i};
      elms.emplace_back(idx, std::make_unique<HelloTyped<int>>(InitialConsTag{}));
    }
  } else if (this_node % 2 == 1) {
    for (int i = this_node*num_per; i < (this_node+1)*num_per; i++) {
      vt::Index1D idx{i};
      elms.emplace_back(idx, std::make_unique<HelloTyped<double>>(InitialConsTag{}));
    }
  }

  auto proxy = vt::makeCollection<Hello>("Polymorphic Collection")
    .bounds(range)
    .listInsertHere(std::move(elms))
    .wait();

  for (int p = 0; p < 10; p++) {
    vt::runInEpochCollective([&]{ proxy.broadcastCollective<&Hello::doWork>(); });
    vt::runInEpochCollective([&]{ proxy.broadcastCollective<migrateToNext>(); });
    vt::runInEpochCollective([&]{ proxy.broadcastCollective<&Hello::doWork>(); });
  }

  for (int p = 0; p < 10; p++) {
    vt::runInEpochCollective([&]{ proxy.broadcastCollective<&Hello::doWork>(); });
    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;
}
/// [Polymorphic collection example]
