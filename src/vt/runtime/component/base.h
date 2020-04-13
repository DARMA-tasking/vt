/*
//@HEADER
// *****************************************************************************
//
//                                    base.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_BASE_H
#define INCLUDED_VT_RUNTIME_COMPONENT_BASE_H

#include "vt/config.h"

#include <vector>
#include <tuple>

namespace vt { namespace runtime { namespace registry {

using AutoHandlerType = auto_registry::AutoHandlerType;
using RegistryType = std::vector<std::tuple<int,std::vector<int>>>;

inline RegistryType& getRegistry() {
  static RegistryType reg;
  return reg;
}

template <typename ObjT>
struct Registrar {
  Registrar() {
    auto& reg = getRegistry();
    index = reg.size();
    reg.emplace_back(std::make_tuple(index,std::vector<int>{}));
  }
  AutoHandlerType index;
};

template <typename ObjT>
struct Type {
  static AutoHandlerType const idx;
};

template <typename ObjT>
AutoHandlerType const Type<ObjT>::idx = Registrar<ObjT>().index;

inline std::vector<int>& getIdx(AutoHandlerType han) {
  return std::get<1>(getRegistry().at(han));
}

template <typename ObjT>
inline AutoHandlerType makeIdx() {
  return Type<ObjT>::idx;
}

}}} /* end namespace vt::runtime::registry */



namespace vt { namespace runtime { namespace component {

template <typename... Args>
struct AddDep;

template <typename U, typename... Args>
struct AddDep<U, Args...> {
  static void add(registry::AutoHandlerType t) {
    auto u = registry::makeIdx<U>();
    registry::getIdx(t).push_back(u);
    AddDep<Args...>::add(t);
  }
};

template <>
struct AddDep<> {
  static void add(registry::AutoHandlerType t) {
  }
};


struct ComponentRegistry {

  template <typename U, typename... Args>
  static void addDep(registry::AutoHandlerType t);

  template <typename T, typename... Deps>
  static void dependsOn() {
    auto t = registry::makeIdx<T>();
    AddDep<Deps...>::add(t);
  }

};


struct Diagnostic {
  virtual void dumpState() = 0;
  // @todo diagnostics
};

struct Bufferable {
  // @todo interface for buffering
};


struct Progressable {
  virtual int progress() = 0;
};

struct BaseComponent : Diagnostic, Bufferable, Progressable {
  virtual bool pollable() = 0;
  virtual void initialize() = 0;
  virtual void finalize() = 0;

  virtual ~BaseComponent() { }
};

template <typename... Deps>
struct DepsPack { };


template <typename T>
struct ComponentTraits {
  template <typename U, typename = decltype(U::construct())>
  static std::true_type test(int);

  template <typename U>
  static std::false_type test(...);

  static constexpr bool hasConstruct = decltype(test<T>(0))::value;
};

template <typename T>
struct Component : BaseComponent {

  Component() = default;

  template <typename... Deps>
  Component(DepsPack<Deps...>) {
    ComponentRegistry::dependsOn<T, Deps...>();
  }

  /// Traits for objgroup components which have a specialized static construct
  template <typename U>
  using hasCons = typename std::enable_if<ComponentTraits<U>::hasConstruct, T>::type;
  template <typename U>
  using hasNoCons = typename std::enable_if<not ComponentTraits<U>::hasConstruct, T>::type;

  template <typename... Args, typename U = T>
  static std::unique_ptr<T> staticInit(Args&&... args, hasCons<U>* = nullptr) {
    return T::construct(std::forward<Args>(args)...);
  }

  template <typename... Args, typename U = T>
  static std::unique_ptr<T> staticInit(Args&&... args, hasNoCons<U>* = nullptr) {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  bool pollable() override {
    return false;
  }

  void initialize() override { }
  void finalize() override { }

  // Default empty progress function
  int progress() override { return 0; }

  void dumpState() override {
    /* here to compile, should be implemented by each component*/
  }
};

template <typename T>
struct PollableComponent : Component<T> {

  bool pollable() override {
    return true;
  }

  // Fail if progress method not overridden by user
  int progress() override {
    vtAssert(false, "PollableComponent should have a progress function");
    return 0;
  }

};



struct Test3 final : PollableComponent<Test3> {
  Test3(int a, int b) : PollableComponent<Test3>() { }

  static std::unique_ptr<Test3> construct(int a, int b) {
    return std::make_unique<Test3>(a, b);
  }

  int progress() override {
    // implement progress function
    return 0;
  };
};

struct Test2 : Component<Test2> {
  Test2() : Component<Test2>() { }
};

struct Test1 : Component<Test1> {
  Test1() : Component<Test1>() { }
};

struct TestA : Component<TestA> {
  TestA() : Component<TestA>() { }
};

struct ComponentPack {

  template <typename T, typename... Deps>
  registry::AutoHandlerType registerComponent(DepsPack<Deps...>) {
    ComponentRegistry::dependsOn<T, Deps...>();
    auto idx = registry::makeIdx<T>();
    registered_components_.push_back(idx);
    return idx;
  }

// private:

//   template <typename T, typename Tuple, typename... Args>
//   std::unique_ptr<T> make(std::tuple<Args...>&& tup) {
//     static constexpr auto size = std::tuple_size<Tuple>::value;
//     static constexpr auto seq = std::make_index_sequence<size>{};
//     return makeImpl<T, Tuple>(std::forward<std::tuple<Args...>>(tup), seq);
//   }

//   template <typename T, typename Tuple, typename... Args, size_t... I>
//   std::unique_ptr<T> makeImpl(std::tuple<Args...>&& tup, std::index_sequence<I...> seq) {
//     return T::template staticInit<Args...>(
//       std::forward<typename std::tuple_element<I,Tuple>::type>(
//         std::get<I>(tup)
//       )...
//     );
//   }

public:

  template <typename T, typename... Args>
  void add(Args&&... args) {
    auto idx = registry::makeIdx<T>();
    construct_components_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(idx),
      std::forward_as_tuple(
        //[this, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        [=]() mutable {
          auto ptr = T::template staticInit<Args...>(std::forward<Args>(args)...);
          // auto ptr = make<T, std::tuple<Args...>>(std::forward<std::tuple<Args...>>(args));
          if (ptr->pollable()) {
            pollable_components_.emplace_back(ptr.get());
          }
          ptr->initialize();
          added_components_.emplace_back(std::move(ptr));
        }
      )
    );
  }

  void topoSort() {
    std::stack<int> order;
    fmt::print("registered={}\n",registered_components_.size());
    auto visited = std::make_unique<bool[]>(registered_components_.size());
    for (std::size_t i = 0; i < registered_components_.size(); i++) {
      if (visited[i] == false) {
        topoSortImpl(i, order, visited.get());
      }
    }

    while (not order.empty()) {
      auto top = order.top();
      fmt::print("top = {}\n", top);
      order.pop();

      auto iter = construct_components_.find(top);
      if (iter != construct_components_.end()) {
        fmt::print("found {}\n", top);
        iter->second();
      } else {
        fmt::print("not found {}\n", top);
      }
    }
  }

private:
  void topoSortImpl(int v, std::stack<int>& order, bool* visited) {
    fmt::print("impl v={}\n",v);
    visited[v] = true;

    auto v_list = registry::getIdx(v);
    for (auto&& vp : v_list) {
      if (not visited[vp]) {
        topoSortImpl(vp, order, visited);
      }
    }

    order.push(v);
  }

  void finalize() {
  }

private:
  std::vector<registry::AutoHandlerType> registered_components_;
  std::unordered_map<registry::AutoHandlerType, ActionType> construct_components_;
  std::vector<std::unique_ptr<BaseComponent>> added_components_;
  std::vector<Progressable*> pollable_components_;
};

void run() {
  ComponentPack p;
  p.registerComponent<TestA>(DepsPack<>{});
  p.registerComponent<Test1>(DepsPack<>{});
  p.registerComponent<Test2>(DepsPack<Test3,TestA>{});
  p.registerComponent<Test3>(DepsPack<Test1,TestA>{});


  // Registrations
  // p.registerComponent<MemoryUsage>(DepsPack<>{});
  // p.registerComponent<Registry>(DepsPack<>{});
  // p.registerComponent<Pool>(DepsPack<>{});
  // p.registerComponent<Event>(DepsPack<>{});
  // p.registerComponent<ActiveMessenger>(DepsPack<Event, Pool, Registry>{});
  // p.registerComponent<ObjGroup>(DepsPack<ActiveMessenger, Pool>{});
  // p.registerComponent<Trace>(DepsPack<Scheduler, ActiveMessenger>{});
  // p.registerComponent<Scheduler>(DepsPack<MemoryUsage>{});
  // p.registerComponent<TerminationDetector>(DepsPack<ActiveMessenger>{});
  // p.registerComponent<CollectiveAlg>(DepsPack<ActiveMessenger>{});
  // p.registerComponent<GroupManager>(DepsPack<ActiveMessenger, CollectiveAlg>{});
  // p.registerComponent<PipeManager>(DepsPack<ActiveMessenger, CollectiveAlg, ObjGroup, CollectionManager>{});
  // p.registerComponent<RDMAManager>(DepsPack<ActiveMessenger>{});
  // p.registerComponent<Param>(DepsPack<ActiveMessenger>{});
  // p.registerComponent<Sequencer>(DepsPack<ActiveMessenger>{});
  // p.registerComponent<SequencerVirtual>(DepsPack<VirtualContextManager, Sequencer>{});
  // p.registerComponent<VirtualContextManager>(DepsPack<ActiveMessenger, Scheduler>{});
  // p.registerComponent<CollectionManager>(DepsPack<ActiveMessenger, GroupManager, Scheduler>{});
  // p.registerComponent<HandleRDMA>(DepsPack<ActiveMessenger, CollectionManager, ObjGroup>{});



  p.add<Test1>();
  p.add<Test2>();
  p.add<Test3>(10, 20);
  p.add<TestA>();

  p.topoSort();


  auto t1 = registry::makeIdx<Test1>();
  auto t2 = registry::makeIdx<Test2>();
  auto t3 = registry::makeIdx<Test3>();
  auto t4 = registry::makeIdx<TestA>();

  fmt::print("Test 1={}, 2={}, 3={}, A={}\n", t1, t2, t3, t4);


  auto v1 = registry::getIdx(t1);
  auto v2 = registry::getIdx(t2);
  auto v3 = registry::getIdx(t3);

  for (auto&& elm : v1) {
    fmt::print("Test1={}, deps={}\n", t1, elm);
  }
  for (auto&& elm : v2) {
    fmt::print("Test2={}, deps={}\n", t2, elm);
  }
  for (auto&& elm : v3) {
    fmt::print("Test3={}, deps={}\n", t3, elm);
  }
}

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_BASE_H*/
