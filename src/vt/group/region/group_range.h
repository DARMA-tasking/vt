/*
//@HEADER
// ************************************************************************
//
//                          group_range.h
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

#if !defined INCLUDED_GROUP_REGION_GROUP_RANGE_H
#define INCLUDED_GROUP_REGION_GROUP_RANGE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/group/group_common.h"
#include "vt/group/region/group_region.h"

namespace vt { namespace group { namespace region {

struct RangeData;

struct Range : Region {
  Range(
    BoundType const& in_lo, BoundType const& in_hi, BoundType const& stride = 1
  );
  Range(Range const& in_other, BoundType in_remove_extent);

  Range(Range const&) = default;
  Range(Range&&) = default;
  Range& operator=(Range const&) = default;

  virtual SizeType getSize() const override;
  virtual void sort() override;
  virtual bool contains(NodeType const& node) override;
  virtual ListType const& makeList() override;
  virtual bool isList() const override;
  virtual BoundType head() const override;
  virtual RegionUPtrType tail() const override;
  virtual SplitRegionType split() const override;
  virtual RegionUPtrType copy() const override;
  virtual void splitN(int nsplits, ApplyFnType apply) const override;

  friend struct RangeData;

private:
  BoundType const lo_ = uninitialized_destination;
  BoundType const hi_ = uninitialized_destination;
  BoundType const stride_ = 1;
  bool made_list_ = false;
  ListType list_;
};

/*
 * A wrapper class for putting range data in a byte-copyable message w/o
 * virtualization and inheritance
 */
struct RangeData {
  using BoundType = NodeType;
  using ListType  = std::vector<BoundType>;

  RangeData() = default;
  RangeData(Range const& r)
    : lo_(r.lo_),
      hi_(r.hi_),
      stride_(r.stride_)
  {
    vtAssertExpr(not r.made_list_);
  }

public:
  Range getRange() const {
    return Range(lo_, hi_, stride_);
  }

private:
  BoundType lo_ = uninitialized_destination;
  BoundType hi_ = uninitialized_destination;
  BoundType stride_ = 1;
};

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_RANGE_H*/
