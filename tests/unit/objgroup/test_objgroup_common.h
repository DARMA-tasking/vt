/*
//@HEADER
// ************************************************************************
//
//                     test_objgroup_common.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "test_parallel_harness.h"
#include "vt/transport.h"

#if !defined INCLUDED_TEST_OBJGROUP_COMMON_H
#define INCLUDED_TEST_OBJGROUP_COMMON_H

namespace vt { namespace tests { namespace unit {

struct MyMsg : vt::Message {

  MyMsg() : from_(vt::theContext()->getNode()) {}
  vt::NodeType from_ = vt::uninitialized_destination;
};

struct SysMsg : vt::collective::ReduceTMsg<int> {
  SysMsg() = delete;
  explicit SysMsg(int in_num)
    : vt::collective::ReduceTMsg<int>(in_num)
  {}
};

struct MyObjA {

  MyObjA() : id_(++next_id) {}
  MyObjA(const MyObjA& obj) = delete;
  MyObjA& operator=(const MyObjA& obj) = delete;
  MyObjA(MyObjA&&) noexcept = default;
  MyObjA& operator=(MyObjA&& obj) noexcept = default;
  ~MyObjA() = default;

  void handler(MyMsg* msg) {
    debug_print(
      objgroup, node,
      "MyObjA: received message from:{} for group:{}, next_id:{}\n",
      msg->from_, id_, next_id
    );
    recv_++;
  }

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
    debug_print(
      objgroup, node,
      "MyObjB: received message from:{} for group:{} with data:{} \n",
      msg->from_, id_, data_
    );
    recv_++;
  }

  int id_ = -1;
  int recv_ = 0;
  int data_ = 0;
  static int next_id;
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

struct VecMsg : vt::collective::ReduceTMsg<VectorPayload> {
  VecMsg() = default;

  explicit VecMsg(int in_num) : ReduceTMsg<VectorPayload>() {
    auto& vec = getVal().vec_;
    vec.push_back(in_num);
    vec.push_back(in_num + 1);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    ReduceTMsg<VectorPayload>::invokeSerialize(s);
  }
};

/*static*/ int MyObjA::next_id = 0;
/*static*/ int MyObjB::next_id = 0;
}}} // end namespace vt::tests::unit

#endif /*INCLUDED_TEST_OBJGROUP_COMMON_H*/