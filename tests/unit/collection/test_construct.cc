
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

#include <cstdint>
#include <tuple>
#include <string>

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

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;
using namespace vt::vrt;
using namespace vt::vrt::collection;

using TestIndex = ::vt::index::Index1D;
using ParamType = int32_t;

// template <typename Tuple, std::size_t val=std::tuple_size<Tuple>::value>
// struct PrintTuple {
//   static void print(Tuple t) {}
// };

// template <typename Tuple>
// struct PrintTuple<Tuple, 1> {
//   static void print(Tuple t) {
//     ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
//   }
// };

static constexpr char*   magic_string_  = "test_string123";
static constexpr int32_t magic_int32_t_ = 29;
static constexpr int64_t magic_int64_t_ = 0xFFFFFEEDFFFFFAAD;

template <typename Tuple, typename=void>
struct ConstructTuple {
  static Tuple construct() { return Tuple{}; }
  static void print(Tuple t) { }
  static void isCorrect(Tuple t) { }
};

template <typename Tuple>
struct ConstructTuple<
  Tuple,
  typename std::enable_if_t<std::is_same<Tuple,std::tuple<std::string>>::value>
>{
  static Tuple construct() {
    return std::make_tuple(std::string(magic_string_));
  }
  static void print(Tuple t) {
    ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
  }
  static void isCorrect(Tuple t) {
    EXPECT_EQ(magic_string_, std::get<0>(t));
  }
};

template <typename Tuple>
struct ConstructTuple<
  Tuple,
  typename std::enable_if_t<std::is_same<Tuple,std::tuple<int32_t>>::value>
>{
  static Tuple construct() { return std::make_tuple(magic_int32_t_); }
  static void print(Tuple t) {
    ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
  }
  static void isCorrect(Tuple t) {
    EXPECT_EQ(magic_int32_t_, std::get<0>(t));
  }
};

template <typename Tuple>
struct ConstructTuple<
  Tuple,
  typename std::enable_if_t<std::is_same<Tuple,std::tuple<int64_t>>::value>
>{
  static Tuple construct() { return std::make_tuple(magic_int64_t_); }
  static void print(Tuple t) {
    ::fmt::print("{}: val={}\n",theContext()->getNode(),std::get<0>(t));
  }
  static void isCorrect(Tuple t) {
    EXPECT_EQ(magic_int64_t_, std::get<0>(t));
  }
};



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
    ConstructTuple<ParamType>::print(std::make_tuple(args...));
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
    ConstructTuple<ParamType>::print(std::make_tuple(args...));
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
    ConstructTuple<ParamType>::print(std::make_tuple(args...));
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
  multi_param_idx_fst_    ::TestCol<std::string>
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

// template <typename Tuple>
// struct PrintParams {
//   static void printTup(Tuple args) { return print(args); }
//   template <typename... Args>
//   static void print(std::tuple<Args...> args) {
//     return unpack(args,std::index_sequence_for<Args...>{});
//   }
//   template <std::size_t ...I>
//   static void unpack(Tuple args, std::index_sequence<I...>) {
//     ::fmt::print("val={}\n",std::get<I>(args)...);
//   }
// };

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
