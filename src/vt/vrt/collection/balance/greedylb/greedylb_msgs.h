/*
//@HEADER
// *****************************************************************************
//
//                               greedylb_msgs.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_MSGS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_MSGS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_types.h"
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/messaging/message.h"

#include <unordered_map>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct GreedyPayload : GreedyLBTypes {
  GreedyPayload() = default;
  GreedyPayload(ObjSampleType const& in_sample, LoadType const& in_profile)
    : sample_(in_sample)
  {
    auto const& this_node = theContext()->getNode();
    auto iter = load_profile_.find(this_node);
    auto end_iter = load_profile_.end();
    vtAssert(iter == end_iter, "Must not exist");
    load_profile_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(this_node),
      std::forward_as_tuple(in_profile)
    );
  }

  friend GreedyPayload operator+(GreedyPayload ld1, GreedyPayload const& ld2) {
    auto& sample1 = ld1.sample_;
    auto const& sample2 = ld2.sample_;
    for (auto&& elm : sample2) {
      auto const& bin = elm.first;
      auto const& entries = elm.second;
      for (auto&& entry : entries) {
        sample1[bin].push_back(entry);
      }
    }
    auto& load1 = ld1.load_profile_;
    auto const& load2 = ld2.load_profile_;
    for (auto&& elm : load2) {
      auto const& proc = elm.first;
      auto const& load = elm.second;
      vtAssert(load1.find(proc) == load1.end(), "Must not exist");
      load1[proc] = load;
    }
    return ld1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | sample_ | load_profile_;
  }

  ObjSampleType const& getSample() const { return sample_; }
  ObjSampleType&& getSampleMove() { return std::move(sample_); }
  LoadProfileType const& getLoadProfile() const { return load_profile_; }
  LoadProfileType&& getLoadProfileMove() { return std::move(load_profile_); }

protected:
  LoadProfileType load_profile_;
  ObjSampleType sample_;
};

struct GreedyCollectMsg : GreedyLBTypes, collective::ReduceTMsg<GreedyPayload> {
  using MessageParentType = collective::ReduceTMsg<GreedyPayload>;
  vt_msg_serialize_required(); // prev. serialize(1)

  GreedyCollectMsg() = default;
  GreedyCollectMsg(ObjSampleType const& in_load, LoadType const& in_profile)
    : collective::ReduceTMsg<GreedyPayload>(GreedyPayload{in_load,in_profile})
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
  }

  ObjSampleType const& getLoad() const {
    return collective::ReduceTMsg<GreedyPayload>::getConstVal().getSample();
  }

  ObjSampleType&& getLoadMove() {
    return collective::ReduceTMsg<GreedyPayload>::getVal().getSampleMove();
  }
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_MSGS_H*/
