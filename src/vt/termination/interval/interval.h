/*
//@HEADER
// ************************************************************************
//
//                          interval.h
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

#if !defined INCLUDED_VT_TERMINATION_INTERVAL_INTERVAL_H
#define INCLUDED_VT_TERMINATION_INTERVAL_INTERVAL_H

#include "vt/config.h"

namespace vt { namespace term { namespace interval {

/*
 * Discrete interval for encoding compressed sets (DIETs). Inclusive in
 * representation.
 */

template <
  typename DomainT,
  typename CompareT = std::less<DomainT>,
  DomainT sentinel = DomainT()
>
struct Interval {
  using IntervalType = Interval<DomainT, CompareT, sentinel>;
  using DomainType   = DomainT;

  Interval() = default;
  Interval(Interval const&) = default;
  Interval(Interval&&) = default;
  Interval& operator=(Interval const&) = default;

  explicit Interval(DomainT const& val) : lb_(val), ub_(val) { }
  Interval(DomainT const& lb, DomainT const& ub) : lb_(lb), ub_(ub) { }

public:
  DomainT lower() const { return lb_; }
  DomainT upper() const { return ub_; }
  DomainT width() const { return (ub_-lb_)+1; }
  bool    valid() const { return lb_ <= ub_; }

  DomainT get(std::size_t i) const {
    vtAssert(lb_ + i <= ub_, "Must be in interval range");
    return lb_ + i;
  }

  void setUpper(DomainT const& val) {
    ub_ = val;
    vtAssert(val >= lb_, "Upper must be greater than lower bound");
    vtAssert(valid(),    "Interval must remain valid");
  }

  void setLower(DomainT const& val) {
    lb_ = val;
    vtAssert(ub_ >= val, "Lower must be less than upper bound");
    vtAssert(valid(),    "Interval must remain valid");
  }

  bool in(DomainT const& val) const {
    return val >= lb_ and val <= ub_;
  }

  enum struct PositionEnum : int {
    TangentLeft = -1,
    NotTangent   = 0,
    TangentRight = 1
  };

  /*
   * Determine if a input interval is tangent to this interval within a
   * specified gap.
   *
   *        TangentLeft                   TangentRight
   *       |------i-----| lb_        ub_ |------i-----|
   *                      |----this----|
   *                     ^              ^
   *                    gap            gap
   */

  PositionEnum tangent(IntervalType const& i, DomainT gap = 1) const {
    if (lb_ == i.ub_ + gap) {
      return PositionEnum::TangentLeft;
    } else if (ub_ == i.lb_ - gap) {
      return PositionEnum::TangentRight;
    } else {
      return PositionEnum::NotTangent;
    }
  }

  void join(IntervalType const& i, PositionEnum pos) {
    switch (pos) {
    case PositionEnum::TangentLeft:
      vtAssertExpr(lb_ == i.upper() + 1);
      lb_ = std::min<DomainT>(lb_, i.lower());
      break;
    case PositionEnum::TangentRight:
      vtAssertExpr(ub_ == i.lower() - 1);
      ub_ = std::max<DomainT>(ub_, i.upper());
      break;
    default:
      vtAssert(false, "Must have tangent position to join");
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | lb_ | ub_;
  }

  bool operator==(IntervalType const& i) const { return i.lb_ == lb_ and i.ub_ == ub_; }
  bool operator!=(IntervalType const& i) const { return !(*this == i); }

public:
  template <typename IntT, typename IntU>
  static bool intersects(IntT&& i1, IntU&& i2) {
    bool ret = i1.lower() <= i2.upper() and i2.lower() <= i1.upper();
    debug_print(
      gen, node,
      "Interval intersects: i1={}, i2={}, op:{}<={}={} and {}<={}={} => {}\n",
      i1, i2,
      i1.lower(), i2.upper(), i1.lower() <= i2.upper(),
      i2.lower(), i1.upper(), i2.lower() <= i1.upper(),
      ret
    );
    return ret;
  }

  template <typename IntT, typename IntU>
  static bool less(IntT&& i1, IntU&& i2) {
    return i1.lower() < i2.lower() and i1.upper() < i2.lower();
  }

  friend bool operator<(IntervalType const& i1, IntervalType const& i2);

  template <typename T, typename U, DomainT v>
  friend std::ostream& operator<<(std::ostream& os, Interval<T,U,v> const& i) {
    os << "itv[" << i.lower() << "," << i.upper() << "]";
    return os;
  }

private:
  DomainT lb_ = sentinel;
  DomainT ub_ = sentinel;
};

template <typename DomainT, typename CompareT, DomainT sentinel>
bool operator<(
  Interval<DomainT, CompareT, sentinel> const& i1,
  Interval<DomainT, CompareT, sentinel> const& i2
) {
  return Interval<DomainT, CompareT, sentinel>::less(i1,i2);
}

template <typename DomainT>
struct IntervalCompare {
  using IntervalType = Interval<DomainT>;
  bool operator()(IntervalType const& i1, IntervalType const& i2) const {
    return IntervalType::less(i1,i2);
  }
};

}}} /* end namespace vt::term::interval */

namespace vt {

template <typename DomainT, typename CompareT = std::less<DomainT>>
using Interval = term::interval::Interval<DomainT, CompareT>;

} /* end namespace vt */

#endif /*INCLUDED_VT_TERMINATION_INTERVAL_INTERVAL_H*/
