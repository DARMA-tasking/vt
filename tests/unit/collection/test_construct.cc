
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cstdint>
#include <tuple>
#include <string>

#define PRINT_CONSTRUCTOR_VALUES 1

namespace serdes {
template <typename Serializer>
void serialize(Serializer& s, std::string& str) {
  typename std::string::size_type str_size = str.length();
  s | str_size;
  str.resize(str_size);
  for (auto&& elm : str) {
    s | elm;
  }
}
} /* end namespace serdes */

namespace test_data {
struct A {
  int32_t a,b,c;
  friend std::ostream& operator<<(std::ostream&os, A const& d) {
    return os << d.a << "-" << d.b << "-" << d.c;
  }
  friend bool operator==(A const& c1,A const& c2) {
    return c1.a == c2.a && c1.b == c2.b && c1.c == c2.c;
  }
};
struct B {
  std::string str;
  template <typename Serializer>
  void serialize(Serializer& s) {
    s | str;
  }
  friend std::ostream& operator<<(std::ostream&os, B const& d) {
    return os << d.str;
  }
  friend bool operator==(B const& c1,B const& c2) {
    return c1.str == c2.str;
  }
};
struct C {
  std::string str;
  int64_t a;
  template <typename Serializer>
  void serialize(Serializer& s) {
    s | str;
    s | a;
  }
  friend std::ostream& operator<<(std::ostream&os, C const& d) {
    return os << d.str << '-' << d.a;
  }
  friend bool operator==(C const& c1,C const& c2) {
    return c1.str == c2.str && c1.a == c2.a;
  }
};
} /* end namespace test_data */

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;
using namespace vt::vrt;
using namespace vt::vrt::collection;

using TestIndex = ::vt::index::Index1D;
using ParamType = int32_t;

static char         magic_string_[]  = "test_string123";
static int32_t      magic_int32_t_   = 29;
static int64_t      magic_int64_t_   = 0xFFFFFEEDFFFFFAAD;
static test_data::A magic_A_t_       = test_data::A{28,2773,12};
static test_data::B magic_B_t_       = test_data::B{"the-ultimate-test"};
static test_data::C magic_C_t_       = test_data::C{"test123",34};

template <typename Tuple, typename=void>
struct ConstructTuple {
  static Tuple construct() { return Tuple{}; }
  static void print(Tuple t) { }
  static void isCorrect(Tuple t) { }
};

#define CONSTRUCT_TUPLE_TYPE(PARAM_TYPE,PARAM_NAME)                     \
  template <typename Tuple>                                             \
  struct ConstructTuple<                                                \
    Tuple,                                                              \
    typename std::enable_if_t<                                          \
      std::is_same<Tuple,std::tuple<PARAM_TYPE>>::value                 \
    >                                                                   \
  >{                                                                    \
    static Tuple construct() {                                          \
      return std::make_tuple(PARAM_NAME);                               \
    }                                                                   \
    static void print(Tuple t) {                                        \
      ::fmt::print(                                                     \
        "{}: val={}\n",theContext()->getNode(),std::get<0>(t)           \
      );                                                                \
    }                                                                   \
    static void isCorrect(Tuple t) {                                    \
      EXPECT_EQ(PARAM_NAME, std::get<0>(t));                            \
    }                                                                   \
};

CONSTRUCT_TUPLE_TYPE(std::string,magic_string_);
CONSTRUCT_TUPLE_TYPE(int32_t,magic_int32_t_);
CONSTRUCT_TUPLE_TYPE(int64_t,magic_int64_t_);
CONSTRUCT_TUPLE_TYPE(test_data::A,magic_A_t_);
CONSTRUCT_TUPLE_TYPE(test_data::B,magic_B_t_);
CONSTRUCT_TUPLE_TYPE(test_data::C,magic_C_t_);

// template <typename Tuple>
// struct ConstructTuple<
//   Tuple,
//   typename std::enable_if_t<std::is_same<Tuple,std::tuple<std::string>>::value>
// >{
//   static Tuple construct() {
//     return std::make_tuple(std::string(magic_string_));
//   }
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
//   static void isCorrect(Tuple t) {
//     EXPECT_EQ(magic_string_, std::get<0>(t));
//   }
// };

// template <typename Tuple>
// struct ConstructTuple<
//   Tuple,
//   typename std::enable_if_t<std::is_same<Tuple,std::tuple<int32_t>>::value>
// >{
//   static Tuple construct() { return std::make_tuple(magic_int32_t_); }
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
//   static void isCorrect(Tuple t) {
//     EXPECT_EQ(magic_int32_t_, std::get<0>(t));
//   }
// };

// template <typename Tuple>
// struct ConstructTuple<
//   Tuple,
//   typename std::enable_if_t<std::is_same<Tuple,std::tuple<int64_t>>::value>
// >{
//   static Tuple construct() { return std::make_tuple(magic_int64_t_); }
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
//   static void isCorrect(Tuple t) {
//     EXPECT_EQ(magic_int64_t_, std::get<0>(t));
//   }
// };

// template <typename Tuple>
// struct ConstructTuple<
//   Tuple,
//   typename std::enable_if_t<std::is_same<Tuple,std::tuple<test_data::A>>::value>
// >{
//   static Tuple construct() { return std::make_tuple(magic_A_t_); }
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
//   static void isCorrect(Tuple t) { EXPECT_EQ(magic_A_t_, std::get<0>(t)); }
// };

// template <typename Tuple>
// struct ConstructTuple<
//   Tuple,
//   typename std::enable_if_t<std::is_same<Tuple,std::tuple<test_data::B>>::value>
// >{
//   static Tuple construct() { return std::make_tuple(magic_B_t_); }
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
//   static void isCorrect(Tuple t) { EXPECT_EQ(magic_B_t_, std::get<0>(t)); }
// };

// template <typename Tuple>
// struct ConstructTuple<
//   Tuple,
//   typename std::enable_if_t<std::is_same<Tuple,std::tuple<test_data::C>>::value>
// >{
//   static Tuple construct() { return std::make_tuple(magic_C_t_); }
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
//   static void isCorrect(Tuple t) { EXPECT_EQ(magic_C_t_, std::get<0>(t)); }
// };

struct ConstructHandlers;

namespace default_ {
struct ColMsg;
struct TestCol : Collection<TestCol,TestIndex> {
  using MsgType = ColMsg;
  using ParamType = std::tuple<>;
  TestCol()
    : Collection<TestCol, TestIndex>(),
      constructed_(true)
  { }
  bool isConstructed() const { return constructed_; }
private:
  bool constructed_ = false;
};
struct ColMsg : CollectionMessage<TestCol> { };
} /* end namespace default_ */

namespace index_ {
struct ColMsg;
struct TestCol : Collection<TestCol,TestIndex> {
  using MsgType = ColMsg;
  using ParamType = std::tuple<>;
  TestCol() = default;
  TestCol(TestIndex idx)
    : Collection<TestCol, TestIndex>(),
      constructed_(true)
  { }
  bool isConstructed() const { return constructed_; }
private:
  bool constructed_ = false;
};
struct ColMsg : CollectionMessage<TestCol> { };
} /* end namespace default_ */

struct BaseCol {
  BaseCol() = default;
  explicit BaseCol(bool const in_constructed)
    : constructed_(in_constructed)
  {}
  bool isConstructed() const { return constructed_; }
protected:
  bool constructed_ = false;
};

namespace multi_param_idx_fst_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex>, BaseCol {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  TestCol(Args... args,TestIndex idx)
    : Collection<TestCol, TestIndex>(),
      BaseCol(true)
  {
    #if PRINT_CONSTRUCTOR_VALUES
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {};
} /* end namespace multi_param_idx_fst_ */

namespace multi_param_idx_snd_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex>, BaseCol {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  TestCol(TestIndex idx,Args... args)
    : Collection<TestCol, TestIndex>(),
      BaseCol(true)
  {
    #if PRINT_CONSTRUCTOR_VALUES
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {};
} /* end namespace multi_param_idx_snd_ */

namespace multi_param_no_idx_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex>, BaseCol {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  TestCol(Args... args)
    : Collection<TestCol, TestIndex>(),
      BaseCol(true)
  {
    #if PRINT_CONSTRUCTOR_VALUES
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {};
} /* end namespace multi_param_no_idx_ */

struct ConstructHandlers {
  template <
    typename CollectionT,
    typename MessageT = typename CollectionT::MsgType
  >
  static void handler(MessageT* msg, CollectionT* col) {
    // fmt::print(
    //   "{}: constructed TestCol: idx.x()={}\n",
    //   theContext()->getNode(), col->getInex().x(), print_ptr(col)
    // );
    EXPECT_TRUE(col->isConstructed());
  }
};

template <typename CollectionT>
struct TestConstruct : TestParallelHarness {};

using CollectionTestTypes = testing::Types<
  // default_                ::TestCol,
  // index_                  ::TestCol,
  multi_param_idx_fst_    ::TestCol<int32_t>,
  multi_param_idx_fst_    ::TestCol<std::string>,
  multi_param_idx_fst_    ::TestCol<test_data::C>
  // multi_param_idx_fst_    ::TestCol<int64_t>,
  // multi_param_idx_fst_    ::TestCol<int32_t,int32_t>,
  // multi_param_idx_fst_    ::TestCol<int64_t,int64_t>,
  // multi_param_idx_snd_    ::TestCol<int32_t>,
  // multi_param_idx_snd_    ::TestCol<int64_t>,
  // multi_param_idx_snd_    ::TestCol<int32_t,int32_t>,
  // multi_param_idx_snd_    ::TestCol<int64_t,int64_t>,
  // multi_param_no_idx_     ::TestCol<int32_t>,
  // multi_param_no_idx_     ::TestCol<int64_t>,
  // multi_param_no_idx_     ::TestCol<int32_t,int32_t>,
  // multi_param_no_idx_     ::TestCol<int64_t,int64_t>
>;

TYPED_TEST_CASE_P(TestConstruct);

template <typename ColT, typename Tuple>
struct ConstructParams {
  using IndexType = typename ColT::IndexType;
  using ProxyType = CollectionIndexProxy<ColT,IndexType>;
  static ProxyType constructTup(IndexType idx,Tuple args) {
    return construct(idx,args);
  }
  template <typename... Args>
  static ProxyType construct(IndexType idx, std::tuple<Args...> args) {
    return unpack(idx,args,std::index_sequence_for<Args...>{});
  }
  template <std::size_t ...I>
  static ProxyType unpack(
    IndexType idx, Tuple args, std::index_sequence<I...>
  ) {
    return theCollection()->construct<ColT>(idx,std::get<I>(args)...);
  }
};

TYPED_TEST_P(TestConstruct, test_construct_1) {
  using ColType = TypeParam;
  using MsgType = typename ColType::MsgType;
  using ParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = TestIndex(col_size);
    ParamType args = ConstructTuple<ParamType>::construct();
    auto proxy = ConstructParams<ColType,ParamType>::constructTup(range,args);
    auto msg = makeSharedMessage<MsgType>();
    proxy.template broadcast<
      MsgType,
      ConstructHandlers::handler<ColType,MsgType>
    >(msg);
  }
}

REGISTER_TYPED_TEST_CASE_P(TestConstruct, test_construct_1);

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_1, TestConstruct, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
