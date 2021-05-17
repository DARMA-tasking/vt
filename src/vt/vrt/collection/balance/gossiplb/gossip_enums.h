/*
//@HEADER
// *****************************************************************************
//
//                                  gossiplb.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_ENUMS_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_ENUMS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/gossiplb/criterion.h"

#include <string>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace lb {

template <typename T>
struct EnumConverter {
  using EnumToStrMap = typename std::unordered_map<T, std::string>;
  using StrToEnumMap = typename std::unordered_map<std::string, T>;

  EnumConverter(
    const std::string &par_name,
    const std::string &enum_type,
    const EnumToStrMap &enum_to_str
  ) : enum_to_str_(enum_to_str),
      par_name_(par_name),
      enum_type_(enum_type)
  {
    for (const auto &it : enum_to_str_) {
      str_to_enum_[it.second] = it.first;
    }
  }

  virtual ~EnumConverter() = default;

  std::string getString(T e) const {
    auto it = enum_to_str_.find(e);
    if (it == enum_to_str_.end()) {
      auto err = fmt::format(
        "GossipLB EnumConverter: enum '{}' value '{}' was not found in the map",
        enum_type_, e
      );
      vtAbort(err);
    }
    return it->second;
  }

  T getEnum(std::string s) const {
    auto it = str_to_enum_.find(s);
    if (it == str_to_enum_.end()) {
      auto err = fmt::format(
        "GossipLB: LB argument '{}' value '{}' is not recognized",
        par_name_, s
      );
      vtAbort(err);
    }
    return it->second;
  }

  T getFromSpec(balance::SpecEntry* spec, T def_value) const {
    std::string spec_value = spec->getOrDefault<std::string>(
      par_name_, getString(def_value)
    );
    return getEnum(spec_value);
  }

  EnumToStrMap enum_to_str_;
  StrToEnumMap str_to_enum_;
  std::string par_name_;
  std::string enum_type_;
};

/// Enum for gossiping approach
enum struct InformTypeEnum : uint8_t {
  /**
   * \brief Synchronous sharing of underloaded processor loads
   *
   * The round number is defined at the processor level. This approach
   * propagates known loads after all messages for a round are received,
   * maximizing the amount of information propagated per round, but has a
   * synchronization cost.
   */
  SyncInform  = 0,
  /**
   * \brief Asynchronous sharing of underloaded processor loads
   *
   * The round number is defined at the message level. This approach
   * propagates known loads when the first message for a round is received,
   * avoiding the synchronization cost but delaying the propagation of some
   * information until the following round.
   */
  AsyncInform = 1
};

/// Enum for the order in which local objects are considered for transfer
enum struct ObjectOrderEnum : uint8_t {
  Arbitrary = 0, //< Arbitrary order: iterate as defined by the unordered_map
  /**
   * \brief By element ID
   *
   * Sort ascending by the ID member of ElementIDStruct.
   */
  ElmID     = 1,
  /**
   * \brief Order for the fewest migrations
   *
   * Order starting with the object with the smallest load that can be
   * transferred to drop the processor load below the average, then by
   * descending load for objects with smaller loads, and finally by ascending
   * load for objects with larger loads.
   */
  FewestMigrations = 2,
  /**
   * \brief Order for migrating the objects with the smallest loads
   *
   * Find the object with the smallest load where the sum of its own load and
   * all smaller loads meets or exceeds the amount by which this processor's
   * load exceeds the target load. Order starting with that object, then by
   * descending load for objects with smaller loads, and finally by ascending
   * load for objects with larger loads.
   */
  SmallObjects = 3,
  /**
   * \brief Order by descending load
   */
  LargestObjects = 4
};

/// Enum for how the CMF is computed
enum struct CMFTypeEnum : uint8_t {
  /**
   * \brief Original approach
   *
   * Remove processors from the CMF as soon as they exceed the target (e.g.,
   * processor-avg) load. Use a CMF factor of 1.0/x, where x is the target load.
   */
  Original   = 0,
  /**
   * \brief Compute the CMF factor using the largest processor load in the CMF
   *
   * Do not remove processors from the CMF that exceed the target load until the
   * next iteration. Use a CMF factor of 1.0/x, where x is the greater of the
   * target load and the load of the most loaded processor in the CMF.
   */
  NormByMax  = 1,
  /**
   * \brief Compute the CMF factor using the load of this processor
   *
   * Do not remove processors from the CMF that exceed the target load until the
   * next iteration. Use a CMF factor of 1.0/x, where x is the load of the
   * processor that is computing the CMF.
   */
  NormBySelf = 2,
  /**
   * \brief Narrow the CMF to only include processors that can accommodate the
   * transfer
   *
   * Use a CMF factor of 1.0/x, where x is the greater of the target load and
   * the load of the most loaded processor in the CMF. Only include processors
   * in the CMF that will pass the chosen Criterion for the object being
   * considered for transfer.
   */
  NormByMaxExcludeIneligible = 3,
};

/// Enum for determining fanout and rounds
enum struct KnowledgeEnum : uint8_t {
  /**
   * \brief User defined values of fanout and rounds
   *
   * The fanout and rounds must be set explicitly
   */
  UserDefined = 0,
  /**
   * \brief Full information
   *
   * The fanout will be as large as possible, with only one round
   */
  Complete    = 1,
  /**
   * \brief Choose rounds and/or fanout based on log rule
   *
   * The relationship between rounds and fanout will be approximately
   * rounds = log(num_ranks)/log(fanout).
   */
  Log         = 2
};

}}}} /* end namespace vt::vrt::collection::lb */

namespace std {

using CriterionEnum = ::vt::vrt::collection::lb::CriterionEnum;

template <>
struct hash<CriterionEnum> {
  size_t operator()(CriterionEnum const& in) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(in));
  }
};

using InformTypeEnum = ::vt::vrt::collection::lb::InformTypeEnum;

template <>
struct hash<InformTypeEnum> {
  size_t operator()(InformTypeEnum const& in) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(in));
  }
};

using ObjectOrderEnum = ::vt::vrt::collection::lb::ObjectOrderEnum;

template <>
struct hash<ObjectOrderEnum> {
  size_t operator()(ObjectOrderEnum const& in) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(in));
  }
};

using CMFTypeEnum = ::vt::vrt::collection::lb::CMFTypeEnum;

template <>
struct hash<CMFTypeEnum> {
  size_t operator()(CMFTypeEnum const& in) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(in));
  }
};

using KnowledgeEnum = ::vt::vrt::collection::lb::KnowledgeEnum;

template <>
struct hash<KnowledgeEnum> {
  size_t operator()(KnowledgeEnum const& in) const {
    return std::hash<uint8_t>()(static_cast<uint8_t>(in));
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_ENUMS_H*/
