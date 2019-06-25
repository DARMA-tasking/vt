/*
//@HEADER
// ************************************************************************
//
//                          discrete_interval.h
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

#if !defined INCLUDED_VT_TERMINATION_INTERVAL_DISCRETE_INTERVAL_H
#define INCLUDED_VT_TERMINATION_INTERVAL_DISCRETE_INTERVAL_H

#include "vt/config.h"

#include <algorithm>

namespace vt { namespace term { namespace interval {

/*
 * Discrete interval tree for encoding non-overlapping integer ranges in a tree
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

  Interval(DomainT const& val) : lb_(val), ub_(val) { }
  Interval(DomainT const& lb, DomainT const& ub) : lb_(lb), ub_(ub) { }

public:
  DomainT lower() const { return lb_; }
  DomainT upper() const { return ub_; }
  DomainT width() const { return (ub_-lb_)+1; }
  bool    valid() const { lb_ <= ub_; }

  template <typename DomainU>
  bool in(DomainU&& val) const {
    return val >= lb_ and val <= ub_;
  }

  void expandBy(DomainT const& val) {
    if (val > 0) {
      ub_ += val;
    } else {
      lb_ -= val;
    }
  }

  enum struct PositionEnum : int {
    TangentLeft = -1,
    NotTangent   = 0,
    TangentRight = 1
  };

  /*
   * Determine if a input interval is tangent to this interval within a
   * specified grap.
   *
   *        TangentLeft                   TangentRight
   *       |------i-----| lb_        ub_ |------i-----|
   *                      |----this----|
   *                     ^              ^
   *                    gap            gap
   */

  PositionEnum tangent(IntervalType const& i, DomainT gap = 1) {
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

public:
  template <typename IntT, typename IntU>
  static bool less(IntT&& i1, IntU&& i2) {
    return i1.lower() <= i2.upper() and i2.lower() <= i1.upper();
  }

  friend bool operator<(IntervalType const& i1, IntervalType const& i2);

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


// template <typename T>
// struct Node {
//   template <typename T>
//   using PtrType = std::unique_ptr<Node<T>>;

//   Node() = default;
//   explicit Node(T z) : x_(z), y_(z) { }

//   bool inLocal(T z) const { return z >= x_ and z <= y; }

//   bool in(T z) {
//     if (inLocal(z)) {
//       return true;
//     } else {
//       if (z < x_) {
//         return c1_ ? c1_->in(z) : false;
//       } else {
//         return c2_ ? c2_->in(z) : false;
//       }
//     }
//   }

//   void insert(T z, bool new_insert = false) {
//     if (inLocal(z)) {
//       vtAssertInfo(
//         not new_insert, "Element insertion in interval exists", z, x_, y_
//       );
//       return;
//     }

//     if (z < x_) {
//       // test expand OR create node
//       c1_->insert(z);
//     } else {
//       // test expand OR create node
//       c2_->insert(z);
//     }
//   }

//   bool hasChildren() const {
//     return c1 != nullptr or c2 != nullptr;
//   }

//   void testInvariants() const {
//     vtAssertExpr(x_ <= y_);
//   }

// private:
//   T x_ = 0;
//   T y_ = 0;
//   PtrType c1_ = nullptr;
//   PtrType c2_ = nullptr;
// };

template <
  typename DomainT,
  typename CompareT,
  template <class, class> class IntervalT,
  template <class, class> class OrderedSetT
>
struct IntervalSetBase {
  using DomainType          = DomainT;
  using IntervalType        = IntervalT<DomainT, CompareT>;
  using IntervalCompareType = IntervalCompare<DomainT>;
  using OrderedSetType      = OrderedSetT<IntervalType, IntervalCompareType>;
  using IteratorType        = typename OrderedSetType::iterator;
  using PositionType        = typename OrderedSetType::PositionEnum;

  IntervalSetBase() : iter_(set_.end()) { }
  IntervalSetBase(IntervalSetBase const&) = default;
  IntervalSetBase(IntervalSetBase&&) = default;

  template <
    typename DomainU,
    typename = std::enable_if_t<
      std::is_same<std::remove_reference_t<DomainU>, DomainT>::value
    >
  >
  IteratorType insert(DomainU&& val) {
    IntervalType i(std::forward<DomainU>(val));

    // Expand the global bounds in this interval set
    updateGlobal(i);

    return insertSet(iter_,std::move(i));
  }

  template <
    typename DomainU,
    typename = std::enable_if_t<
      std::is_same<std::remove_reference_t<DomainU>, DomainT>::value
    >
  >
  IteratorType insert(IteratorType it, DomainU&& val) {
    vtAsserExpr(it not_eq set_.end());
    IntervalType i(std::forward<DomainU>(val));

    // Expand the global bounds in this interval set
    updateGlobal(i);

    // If the val is tangent to the hint iterator, directly update
    auto pos = it->tangent(i);
    if (pos not_eq PositionType::NotTangent) {
      it->join(i, pos);
      return it;
    }

    return insertSet(it,std::move(i));
  }

  void clear() {
    set_.clear();
    lb_ = ub_ = 0;
    iter_ = set_.end();
  }

  DomainT range() const { return ub_ - lb_; }
  DomainT lower() const { return lb_; }
  DomainT upper() const { return ub_; }
  bool    empty() const { return set_.size() == 0; }

private:

  template <
    typename DomainU,
    typename = std::enable_if_t<
      std::is_same<std::remove_reference_t<DomainU>, DomainT>::value
    >
  >
  IteratorType insertSet(IteratorType it, IntervalType i) {
    // Insert into the set
    auto ret = set_.emplace_hint(it,std::move(i));
    vtAssert(ret.second,                  "Should be a valid insert");
    vtAssert(ret.first not_eq set_.end(), "Must be valid insert---live iterator");

    // Fuse interval set elements that are now tangent after this insertion
    return iter_ = join(ret.first);
  }

  void updateGlobal(IntervalType const& i) {
    // Expand the global bounds in this interval set
    lb_ = std::min<DomainT>(lb_, i.lower());
    ub_ = std::max<DomainT>(ub_, i.upper());
  }

  IteratorType join(IteratorType it) {
    auto it2 = joinLeft(it);
    return joinRight(it2);
  }

  IteratorType joinLeft(IteratorType it) {
    vtAssertExpr(it not_eq set_.end());
    if (it not_eq set_.begin()) {
      auto prev = std::prev(it);
      if (it->tangent(prev) == PositionType::TangentLeft) {
        it->join(prev, PositionType::TangentLeft);
        set_.erase(prev);
      }
    }
    return it;
  }

  IteratorType joinRight(IteratorType it) {
    vtAssertExpr(it not_eq set_.end());
    auto next = std::next(it);
    if (it not_eq set_.end()) {
      if (it->tangent(next) == PositionType::TangentRight) {
        it->join(next, PositionType::TangentRight);
        set_.erase(next);
      }
    }
    return it;
  }

private:
  // The lower bound for the entire set
  DomainT lb_         = 0;
  // The upper bound for the entire set
  DomainT ub_         = 0;
  // The set of all the interval ranges
  OrderedSetType set_ = {};
  // The previous insert iterator for quick lookup
  IteratorType iter_  = {};
};

}}} /* end namespace vt::term::interval */

#endif /*INCLUDED_VT_TERMINATION_INTERVAL_DISCRETE_INTERVAL_H*/
