
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_MSGS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_MSGS_H

#include "config.h"
#include "vrt/collection/balance/greedylb/greedylb_types.h"
#include "vrt/collection/balance/proc_stats.h"
#include "messaging/message.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

struct GreedyPayload : GreedyLBTypes {
  GreedyPayload() = default;
  explicit GreedyPayload(ObjSampleType const& in_sample)
    : sample_(in_sample)
  {}

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
    return ld1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | sample_;
  }

  ObjSampleType const& getSample() const { return sample_; }
  ObjSampleType&& getSampleMove() { return std::move(sample_); }

protected:
  ObjSampleType sample_;
};

struct GreedyCollectMsg : GreedyLBTypes, collective::ReduceTMsg<GreedyPayload> {
  using LoadType = double;

  GreedyCollectMsg() = default;
  explicit GreedyCollectMsg(ObjSampleType const& in_load)
    : collective::ReduceTMsg<GreedyPayload>(GreedyPayload{in_load})
  { }

  #if greedylb_use_parserdes
    template <typename SerializerT>
    void parserdes(SerializerT& s) {
      s & load_;
    }
  #else
    template <typename SerializerT>
    void serialize(SerializerT& s) {
      ReduceTMsg<GreedyPayload>::invokeSerialize(s);
    }
  #endif

  ObjSampleType const& getLoad() const {
    return collective::ReduceTMsg<GreedyPayload>::getConstVal().getSample();
  }

  ObjSampleType&& getLoadMove() {
    return collective::ReduceTMsg<GreedyPayload>::getVal().getSampleMove();
  }
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_MSGS_H*/
