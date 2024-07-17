/*
//@HEADER
// *****************************************************************************
//
//                            test_objgroup_common.h
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

#if !defined INCLUDED_UNIT_OBJGROUP_TEST_OBJGROUP_COMMON_H
#define INCLUDED_UNIT_OBJGROUP_TEST_OBJGROUP_COMMON_H

#include "test_parallel_harness.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/collective/reduce/allreduce/data_handler.h"

#include <numeric>
#include <vector>

namespace vt { namespace tests { namespace unit {

struct MyMsg : vt::Message {

  MyMsg() : from_(vt::theContext()->getNode()) {}
  vt::NodeType from_ = vt::uninitialized_destination;
};

struct VectorPayload {
  VectorPayload() = default;

  friend VectorPayload operator+(VectorPayload v1, VectorPayload const& v2) {
    for (auto&& elm : v2.vec_) {
      v1.vec_.push_back(elm);
    }
    return v1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec_;
  }

  std::vector<int> vec_;
};

struct MyObjA {

  MyObjA() : id_(++next_id) {}
  MyObjA(const MyObjA& obj) = delete;
  MyObjA& operator=(const MyObjA& obj) = delete;
  MyObjA(MyObjA&&) noexcept = default;
  MyObjA& operator=(MyObjA&& obj) noexcept = default;
  ~MyObjA() = default;

  void handler(MyMsg* msg) {
    vt_debug_print(
      normal, objgroup,
      "MyObjA: received message from:{} for group:{}, next_id:{}\n",
      msg->from_, id_, next_id
    );
    recv_++;
  }

  void handlerOnlyFromSelf(MyMsg* msg) {
    vt_debug_print(
      normal, objgroup,
      "MyObjA: received message from:{} for group:{}, next_id:{}\n", msg->from_,
      id_, next_id);
    if (msg->from_ == ::vt::theContext()->getNode())
      recv_++;
  }

  int accumulateVec(const std::vector<int32_t>& vec) {
    recv_++;

    return std::accumulate(std::begin(vec), std::end(vec), 0);
  }

  std::unique_ptr<int32_t> modifyNonCopyableStruct(std::unique_ptr<int32_t> i) {
    recv_++;
    i = std::make_unique<int32_t>(10);

    return i;
  }

  static inline int32_t total_verify_expected_ = 0;

  template <int test>
  void verifyAllred(int value) {
    auto const n = vt::theContext()->getNumNodes();
    switch (test) {
    case 1: EXPECT_EQ(value, n * (n - 1)/2); break;
    case 2: EXPECT_EQ(value, n * 4); break;
    case 3: EXPECT_EQ(value, n - 1); break;
    default: vtAbort("Failure: should not be reached"); break;
    }
    total_verify_expected_++;
  }

  void verifyAllredVec(std::vector<int> vec) {
    auto final_size = vec.size();
    EXPECT_EQ(final_size, 256);

    auto n = vt::theContext()->getNumNodes();
    auto const total_sum = n * (n - 1)/2;
    for(auto val : vec){
      EXPECT_EQ(val, total_sum);
    }

    total_verify_expected_++;
  }

  void verifyAllredVecPayload(VectorPayload vec) { verifyAllredVec(vec.vec_); }

#if MAGISTRATE_KOKKOS_ENABLED
  void verifyAllredView(Kokkos::View<float*, Kokkos::HostSpace> view) {
    auto final_size = view.extent(0);
    EXPECT_EQ(final_size, 256);

    auto n = vt::theContext()->getNumNodes();
    auto const total_sum = n * (n - 1) / 2;
    Kokkos::parallel_for("InitView", view.extent(0), KOKKOS_LAMBDA(const int i) {
      EXPECT_EQ(view(i), total_sum);
    });

    total_verify_expected_++;
  }
#endif // MAGISTRATE_KOKKOS_ENABLED

  int id_ = -1;
  int recv_ = 0;
  static int next_id;
};

struct MyObjB {

  MyObjB() = delete;
  MyObjB(const MyObjB& obj) = default;
  MyObjB& operator=(const MyObjB& obj) = default;
  MyObjB(MyObjB&&) noexcept = default;
  MyObjB& operator=(MyObjB&& obj) noexcept = default;
  ~MyObjB() = default;

  explicit MyObjB(int in_data) : id_(++next_id), data_(in_data) {}

  void handler(MyMsg* msg) {
    vt_debug_print(
      normal, objgroup,
      "MyObjB: received message from:{} for group:{} with data:{} \n",
      msg->from_, id_, data_
    );
    recv_++;
  }

  int id_ = -1;
  int recv_ = 0;
  int data_ = 0;
  static int next_id;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | id_
      | recv_
      | data_
      | next_id;
  }
};

/*static*/ int MyObjA::next_id = 0;
/*static*/ int MyObjB::next_id = 0;
}}} // end namespace vt::tests::unit

namespace vt::collective::reduce::allreduce {
template<>
class DataHandler<tests::unit::VectorPayload> {
private:
  using DataT = ::vt::tests::unit::VectorPayload;
public:
  using Scalar = typename decltype(DataT::vec_)::value_type;

  static size_t size(const DataT& data) { return data.vec_.size(); }
  static const std::vector<Scalar>& toVec(const DataT& data) { return data.vec_; }
  static DataT fromVec(const std::vector<Scalar>& data) { return DataT{data}; }
  static DataT fromMemory(const Scalar* data, size_t count) {
    return DataT{std::vector<Scalar>(data, data + count)};
  }
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_UNIT_OBJGROUP_TEST_OBJGROUP_COMMON_H*/
