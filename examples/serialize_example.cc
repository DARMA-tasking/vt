
#include "transport.h"
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

  auto* tptr1 = deserialize<std::vector<int>>(buf, buf_size);
  auto& t = *tptr1;

  for (auto&& elm : t) {
    fmt::print("vec: deserialized elm={}\n", elm);
  }
}

void testSerializeUserClass() {
  MyTest my_test_inst{10};

  my_test_inst.print();

  auto serialized = serialize<MyTest>(my_test_inst);

  auto const& buf = serialized->getBuffer();
  auto const& buf_size = serialized->getSize();

  fmt::print("ptr={}, size={}\n", buf, buf_size);

  auto* tptr1 = deserialize<MyTest>(buf, buf_size);
  auto& t = *tptr1;

  t.print();
}

void testSerializeByteUserClass() {
  using Type = MyTest3ByteSerializable;

  Type my_test_inst;

  my_test_inst.print();

  auto serialized = serialize<Type>(my_test_inst);

  auto const& buf = serialized->getBuffer();
  auto const& buf_size = serialized->getSize();

  fmt::print("ptr={}, size={}\n", buf, buf_size);

  auto* tptr1 = deserialize<Type>(buf, buf_size);
  auto& t = *tptr1;

  t.print();
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

  auto tptr = deserialize<serial_type_t>(
    buf2->getBuffer(), s1->getSize()
  );
  auto& t1 = *tptr;

  fmt::print("[[ {}, {} ]]\n", std::get<0>(t1), std::get<1>(t1));
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

  auto tptr = deserialize<serial_type_t>(
    buf2->getBuffer(), s1->getSize()
  );
  auto& t1 = *tptr;

  fmt::print("[[ {}, {} ]]\n", std::get<0>(t1), std::get<1>(t1)[0]);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

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
