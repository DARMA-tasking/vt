/*
//@HEADER
// *****************************************************************************
//
//                        test_component_construction.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include <vt/runtime/component/component.h>
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestComponentConstruction = TestParallelHarness;

////////////////////////////////////////////////////////////////////////////////
// Test dispatch to construct() method with no args using a private constructor
////////////////////////////////////////////////////////////////////////////////

struct MyComponent : runtime::component::Component<MyComponent> {
public:
  struct Tag {};
  explicit MyComponent(Tag) {}
public:
  static std::unique_ptr<MyComponent> construct() {
    return std::make_unique<MyComponent>(Tag{});
  }
  std::string name() override { return "MyComponent"; }
};

TEST_F(TestComponentConstruction, test_component_construct_dispatch_1) {
  using vt::runtime::component::ComponentPack;
  using vt::runtime::component::StartupDeps;
  using vt::runtime::component::RuntimeDeps;

  MyComponent* my_dumb_pointer = nullptr;

  auto p = std::make_unique<ComponentPack>();
  p->registerComponent<MyComponent>(
    &my_dumb_pointer, StartupDeps<>{}, RuntimeDeps<>{}
  );
  p->add<MyComponent>();
  p->construct();

  EXPECT_NE(my_dumb_pointer, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// Test dispatch to construct() method with arguments
////////////////////////////////////////////////////////////////////////////////

struct MyComponentArgs : runtime::component::Component<MyComponentArgs> {
public:
  struct Tag {};
  explicit MyComponentArgs(Tag) {}
public:
  struct MyTag {};
  static std::unique_ptr<MyComponentArgs> construct(
    [[maybe_unused]] int a, int& b, [[maybe_unused]] MyTag&& tag
  ) {
    b = 20;
    return std::make_unique<MyComponentArgs>(Tag{});
  }
  std::string name() override { return "MyComponentArgs"; }
};

TEST_F(TestComponentConstruction, test_component_construct_dispatch_args_2) {
  using vt::runtime::component::ComponentPack;
  using vt::runtime::component::StartupDeps;
  using vt::runtime::component::RuntimeDeps;

  MyComponentArgs* my_dumb_pointer = nullptr;

  int my_int = 10;

  auto p = std::make_unique<ComponentPack>();
  p->registerComponent<MyComponentArgs>(
    &my_dumb_pointer, StartupDeps<>{}, RuntimeDeps<>{},
    10, my_int, typename MyComponentArgs::MyTag{}
  );
  p->add<MyComponentArgs>();
  p->construct();

  EXPECT_NE(my_dumb_pointer, nullptr);
  EXPECT_EQ(my_int, 20);
}

////////////////////////////////////////////////////////////////////////////////
// Test dispatch to construct() method with a move-only argument
////////////////////////////////////////////////////////////////////////////////

struct MyComponentMove : runtime::component::Component<MyComponentMove> {
public:
  struct Tag {};
  explicit MyComponentMove(Tag) {}
public:
  struct MyTag {};
  static std::unique_ptr<MyComponentMove> construct(std::unique_ptr<MyTag> tag) {
    EXPECT_NE(tag, nullptr);
    return std::make_unique<MyComponentMove>(Tag{});
  }
  std::string name() override { return "MyComponentMove"; }
};

TEST_F(TestComponentConstruction, test_component_construct_dispatch_move_3) {
  using vt::runtime::component::ComponentPack;
  using vt::runtime::component::StartupDeps;
  using vt::runtime::component::RuntimeDeps;

  MyComponentMove* my_dumb_pointer = nullptr;

  auto ptr = std::make_unique<typename MyComponentMove::MyTag>();

  auto p = std::make_unique<ComponentPack>();
  p->registerComponent<MyComponentMove>(
    &my_dumb_pointer, StartupDeps<>{}, RuntimeDeps<>{}, std::move(ptr)
  );
  p->add<MyComponentMove>();
  p->construct();

  EXPECT_NE(my_dumb_pointer, nullptr);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Test dependencies -- runtime and startup to ensure they are being followed
////////////////////////////////////////////////////////////////////////////////

struct ComponentA;
struct ComponentB;
struct ComponentC;
struct ComponentD;

namespace comp_ptr {

ComponentA* pointer_a = nullptr;
ComponentB* pointer_b = nullptr;
ComponentC* pointer_c = nullptr;
ComponentD* pointer_d = nullptr;

} /* end namespace comp_ptr */

struct ComponentA : runtime::component::Component<ComponentA> {
  ComponentA() {
    EXPECT_NE(comp_ptr::pointer_b, nullptr);
    EXPECT_NE(comp_ptr::pointer_c, nullptr);
  }

  std::string name() override { return "ComponentA"; }
};

struct ComponentB : runtime::component::Component<ComponentB> {
  ComponentB() {
    EXPECT_NE(comp_ptr::pointer_c, nullptr);
  }

  std::string name() override { return "ComponentB"; }
};

struct ComponentC : runtime::component::Component<ComponentC> {
  void startup() override {
    EXPECT_NE(comp_ptr::pointer_d, nullptr);
  }

  std::string name() override { return "ComponentC"; }
};

struct ComponentD : runtime::component::Component<ComponentD> {
  void startup() override {
    EXPECT_NE(comp_ptr::pointer_b, nullptr);
  }

  std::string name() override { return "ComponentD"; }
};

TEST_F(TestComponentConstruction, test_component_deps_1) {
  using vt::runtime::component::ComponentPack;
  using vt::runtime::component::StartupDeps;
  using vt::runtime::component::RuntimeDeps;

  auto p = std::make_unique<ComponentPack>();
  p->registerComponent<ComponentA>(
    &comp_ptr::pointer_a, StartupDeps<ComponentB, ComponentC>{}, RuntimeDeps<>{}
  );
  p->registerComponent<ComponentB>(
    &comp_ptr::pointer_b, StartupDeps<ComponentC>{}, RuntimeDeps<>{}
  );
  p->registerComponent<ComponentC>(
    &comp_ptr::pointer_c, StartupDeps<>{}, RuntimeDeps<ComponentD>{}
  );
  p->registerComponent<ComponentD>(
    &comp_ptr::pointer_d, StartupDeps<>{}, RuntimeDeps<ComponentB>{}
  );
  p->add<ComponentA>();
  p->construct();

  // We should have all 4 components, just by adding ComponentA
  EXPECT_NE(comp_ptr::pointer_a, nullptr);
  EXPECT_NE(comp_ptr::pointer_b, nullptr);
  EXPECT_NE(comp_ptr::pointer_c, nullptr);
  EXPECT_NE(comp_ptr::pointer_d, nullptr);
}

TEST_F(TestComponentConstruction, test_component_deps_circular_2) {
  theConfig()->vt_throw_on_abort = true;

  using vt::runtime::component::ComponentPack;
  using vt::runtime::component::StartupDeps;
  using vt::runtime::component::RuntimeDeps;

  auto p = std::make_unique<ComponentPack>();
  p->registerComponent<ComponentA>(
    &comp_ptr::pointer_a, StartupDeps<ComponentB>{}, RuntimeDeps<>{}
  );
  p->registerComponent<ComponentB>(
    &comp_ptr::pointer_b, StartupDeps<ComponentC>{}, RuntimeDeps<>{}
  );
  p->registerComponent<ComponentC>(
    &comp_ptr::pointer_c, StartupDeps<ComponentA>{}, RuntimeDeps<ComponentD>{}
  );
  p->add<ComponentA>();

  EXPECT_THROW(p->construct(), std::runtime_error);
}

////////////////////////////////////////////////////////////////////////////////

}}} /* end namespace vt::tests::unit */
