/*
//@HEADER
// ************************************************************************
//
//                        field_wrapper.h
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

#if !defined INCLUDED_UTILS_FIELD_WRAPPER_H
#define INCLUDED_UTILS_FIELD_WRAPPER_H


#include <cstdlib>
#include <unordered_set>


#include "vt/configs/types/types_type.h"


namespace vt { namespace utils {


enum struct fieldName {
  NonRootedEpochSeq,              // Wrapped
  RootedEpochSeq,                 // Wrapped
  CollectiveGroupSeq,
  EventTypeSeq,                   // Wrapped
  GroupSeq,
  ObjGroupSeq,                    // Wrapped
  PipeSeq,                        // Wrapped
  RegistryHandlerCollectiveSeq,
  RegistryHandlerSeq,
  VirtualRemoteSeq,               // Wrapped
  VirtualRequestSeq,              // Wrapped
  VirtualSeq                      // Wrapped
};


template< typename T, typename BitCountType >
constexpr T maxWrap(BitCountType len) {
  return (len == 1) ? 0 : (maxWrap<T,BitCountType>(len-1) << 1) + 1;
};


template<fieldName wrapField, typename FieldType, BitCountType len>
struct FieldWrapper
{

private:

  static constexpr FieldType first_ = static_cast<FieldType>(1);
  static constexpr FieldType last_ = maxWrap<FieldType, BitCountType>(len);

  static FieldType cur_width_;
  static FieldType cur_tail_;

  static std::unordered_set<FieldType> finished_;

  /* --- Member Functions --- */

  static FieldType next(const FieldType &ref);

public:

  static void clean(const FieldType &ref);
  static void increment(FieldType &ref);

};


template<fieldName wrapField, typename FieldType, BitCountType len>
/*static*/ FieldType FieldWrapper<
  wrapField, FieldType, len
  >::cur_width_ = static_cast<FieldType>(0);


template<fieldName wrapField, typename FieldType, BitCountType len>
/*static*/ FieldType FieldWrapper<
  wrapField, FieldType, len
  >::cur_tail_ = first_;


template<fieldName wrapField, typename FieldType, BitCountType len>
/*static*/ std::unordered_set<FieldType> FieldWrapper<
  wrapField, FieldType, len
  >::finished_ = {};


template<fieldName wrapField, typename FieldType, BitCountType len>
/* static */ inline void FieldWrapper<
  wrapField, FieldType, len
  >::increment(FieldType &seqID)
{
  if (cur_width_ == last_) {
    seqID = static_cast<FieldType>(0);
    vt::abort("Out of Free Identifiers", 999);
    return;
  }
  cur_width_ = cur_width_ + 1;
  seqID = next(seqID);
}


template<fieldName wrapField, typename FieldType, BitCountType len>
/* static */ inline FieldType FieldWrapper<
  wrapField, FieldType, len
  >::next(const FieldType &seqID)
{
  if (seqID == last_) {
    return first_;
  }
  return (seqID + 1);
}


template<fieldName wrapField, typename FieldType, BitCountType len>
/* static */ inline void FieldWrapper<
  wrapField, FieldType, len
  >::clean(const FieldType &ref)
{
  finished_.insert(ref);
  if (cur_tail_ == ref) {
    auto kpos = finished_.find(cur_tail_);
    while (kpos != finished_.end()) {
      cur_width_ = cur_width_ - 1;
      finished_.erase(kpos);
      cur_tail_ = FieldWrapper::next(cur_tail_);
      kpos = finished_.find(cur_tail_);
    }
  }
}


}}  // end namespace vt::utils

#endif

