/*
//@HEADER
// *****************************************************************************
//
//                             serialize_example.cc
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

#include "vt/transport.h"
#include <cstdlib>

#include <cassert>
#include <cstdio>
#include <vector>
#include <tuple>
#include <type_traits>

using namespace vt;
using namespace ::serialization::interface;

struct MyTest3ByteSerializable {
  using isByteCopyable = std::true_type;

  int c = 41, d = 29;

  void print() {
    fmt::print("\t MyTest3ByteSerializable: c={}, d={}\n", c, d);
  }
};

struct MyTest2 {
  int c = 41;

  template <typename Serializer>
  void serialize(Serializer& s) {
    fmt::print("MyTest2 serialize\n");
    s | c;
  }

  void print() {
    fmt::print("\t MyTest2: c={}\n", c);
  }
};

struct MyTest {
  int a = 29, b = 31;
  MyTest2 my_test_2;

  MyTest() = default;

  void print() {
    fmt::print("MyTest: a={}, b={}\n", a, b);
    my_test_2.print();
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    fmt::print("MyTest serialize\n");
    s | a;
    s | b;
    s | my_test_2;
  }
};

void testSerializeVector() {
  std::vector<int> vec{10,20,40};

  for (auto&& elm : vec) {
    fmt::print("vec: before elm={}\n", elm);
  }

  auto serialized = serialize(vec);
  auto const& buf = serialized->getBuffer();
  auto const& buf_size = serialized->getSize();

  fmt::print("vec: ptr={}, size={}\n", buf, buf_size);

  auto buffer = new std::vector<int>;
  auto* tptr1 = deserialize<std::vector<int>>(buf, buf_size, buffer);
  auto& t = *tptr1;

  for (auto&& elm : t) {
    fmt::print("vec: deserialized elm={}\n", elm);
  }

  delete buffer;
}

void testSerializeUserClass() {
  MyTest my_test_inst{10};

  my_test_inst.print();

  auto serialized = serialize<MyTest>(my_test_inst);

  auto const& buf = serialized->getBuffer();
  auto const& buf_size = serialized->getSize();

  fmt::print("ptr={}, size={}\n", buf, buf_size);

  auto buffer = new MyTest;
  auto* tptr1 = deserialize<MyTest>(buf, buf_size, buffer);
  auto& t = *tptr1;

  t.print();

  delete buffer;
}

void testSerializeByteUserClass() {
  using Type = MyTest3ByteSerializable;

  Type my_test_inst;

  my_test_inst.print();

  auto serialized = serialize<Type>(my_test_inst);

  auto const& buf = serialized->getBuffer();
  auto const& buf_size = serialized->getSize();

  fmt::print("ptr={}, size={}\n", buf, buf_size);

  auto buffer = new Type;
  auto* tptr1 = deserialize<Type>(buf, buf_size, buffer);
  auto& t = *tptr1;

  t.print();

  delete buffer;
}

void testSerializeTuple() {
  // Tuple test
  using serial_type_t = std::tuple<int, int>;

  serial_type_t tup{10, 20};

  fmt::print("[[ {}, {} ]]\n", std::get<0>(tup), std::get<1>(tup));

  auto s1 = serialize(tup);

  auto const& buf2 = s1;

  fmt::print(
    "ptr={}, val={}, size={}\n",
    buf2->getBuffer(), *reinterpret_cast<int*>(buf2->getBuffer()),
    s1->getSize()
  );

  auto buffer = new serial_type_t;
  auto tptr = deserialize<serial_type_t>(
    buf2->getBuffer(), s1->getSize(), buffer
  );
  auto& t1 = *tptr;

  fmt::print("[[ {}, {} ]]\n", std::get<0>(t1), std::get<1>(t1));

  delete buffer;
}

void testSerializeTupleVector() {
  // Tuple test
  using serial_type_t = std::tuple<int, std::vector<int>>;

  serial_type_t tup{10,{20,30}};

  fmt::print("[[ {}, {} ]]\n", std::get<0>(tup), std::get<1>(tup)[0]);

  auto s1 = serialize(tup);

  auto const& buf2 = s1;

  fmt::print(
    "ptr={}, val={}, size={}\n",
    buf2->getBuffer(), *reinterpret_cast<int*>(buf2->getBuffer()),
    s1->getSize()
  );

  auto buffer = new serial_type_t;
  auto tptr = deserialize<serial_type_t>(
    buf2->getBuffer(), s1->getSize(), buffer
  );
  auto& t1 = *tptr;

  fmt::print("[[ {}, {} ]]\n", std::get<0>(t1), std::get<1>(t1)[0]);

  delete buffer;
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  testSerializeVector();
  testSerializeTuple();
  testSerializeByteUserClass();

  #if HAS_SERIALIZATION_LIBRARY
    testSerializeTupleVector();
    testSerializeUserClass();
  #endif

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
