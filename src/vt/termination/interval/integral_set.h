/*
//@HEADER
// ************************************************************************
//
//                          integral_set.h
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

#if !defined INCLUDED_VT_TERMINATION_INTERVAL_INTEGRAL_SET_H
#define INCLUDED_VT_TERMINATION_INTERVAL_INTEGRAL_SET_H

#include "vt/config.h"
#include "vt/termination/interval/interval.h"

#include <algorithm>
#include <iterator>
#include <set>

namespace vt { namespace term { namespace interval {

/*
 * Discrete interval tree for encoding a set of partially tangent,
 * non-overlapping integral values into interval ranges seamlessly in a
 * tree. Maintains a "DIET" data structure: Discrete Interval Encoding
 * Tree. When inserting contiguous elements in order, the storage (non-bit) and
 * time is constant O(1).
 */

template <
  typename DomainT,
  typename DomainCompareT,
  DomainT sentinel,
  template <class>                 class AllocatorT,
  template <class, class, DomainT> class IntervalT,
  template <class, class, class>   class OrderedSetT
>
struct IntegralSetBase {
  using DomainType          = DomainT;
  using IntervalType        = IntervalT<DomainT, DomainCompareT, sentinel>;
  using CompareType         = IntervalCompare<DomainT>;
  using AllocType           = AllocatorT<IntervalType>;
  using OrderedSetType      = OrderedSetT<IntervalType, CompareType, AllocType>;
  using IteratorType        = typename OrderedSetType::iterator;
  using PositionType        = typename IntervalType::PositionEnum;

  IntegralSetBase() : hint_(set_.end()) { }
  IntegralSetBase(IntegralSetBase const&) = default;
  IntegralSetBase(IntegralSetBase&&) = default;

  template <
    typename DomainU,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<DomainU>>, DomainT
      >::value
    >
  >
  IteratorType insert(DomainU&& val) {
    return insertInterval(IntervalType(std::forward<DomainU>(val)));
  }

  template <
    typename DomainU,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<DomainU>>, DomainT
      >::value
    >
  >
  IteratorType insert(IteratorType it, DomainU&& val) {
    vtAsserExpr(it not_eq set_.end());
    return insertInterval(IntervalType(std::forward<DomainU>(val)));
  }

  template <
    typename IntervalU,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<IntervalU>>, IntervalType
      >::value
    >
  >
  IteratorType insertInterval(IntervalU&& i) {
    // Expand the global bounds in this interval set
    insertGlobal(i);

    // Insert interval into the compressed tree
    return insertSet(hint_,std::move(i));
  }

  template <
    typename IntervalU,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<IntervalU>>, IntervalType
      >::value
    >
  >
  IteratorType insertInterval(IteratorType it, IntervalU&& i) {
    // Expand the global bounds in this interval set
    insertGlobal(i);

    // If the val is tangent to the hint iterator, directly update
    auto pos = it->tangent(i);
    if (pos not_eq PositionType::NotTangent) {
      it->join(i, pos);
      return it;
    }

    // Insert interval into the compressed tree
    return insertSet(it,std::move(i));
  }

  void erase(DomainT const& val) {
    vtAssert(existsGlobal(val), "This element must exist in the set");
    IntervalType j(val);
    auto iter = set_.find(j);
    bool in_set = iter != set_.end();
    debug_print(
      gen, node,
      "OrderedSet: erase: bounds=[{},{}] size={}, comp={}, val={}, in={}\n",
      lb_, ub_, size(), compressedSize(), j, in_set
    );
    vtAssert(in_set, "The element must exist in a interval bucket");
    if (in_set) {
      if (iter->width() == 1) {
        bool invalid_hint = iter == hint_;
        auto ret = set_.erase(iter);
        if (invalid_hint) {
          hint_ = ret;
        }
      } else if (iter->upper() == val) {
        auto& to_split = const_cast<IntervalType&>(*iter);
        to_split.setUpper(iter->upper() - 1);
      } else if (iter->lower() == val) {
        auto& to_split = const_cast<IntervalType&>(*iter);
        to_split.setLower(iter->lower() + 1);
      } else {
        // Slice the interval into two interval nodes
        vtAssert(iter->width() > 2, "Interval width must be greater than 2");
        auto ub = iter->upper();
        auto& to_split = const_cast<IntervalType&>(*iter);
        to_split.setUpper(val - 1);
        IntervalType i(val+1, ub);
        insertSet(iter,std::move(i));
      }
      eraseGlobal(val);
    }
  }

  bool exists(DomainT const& val) const {
    if (not existsGlobal(val)) {
      return false;
    }
    IntervalType i(val);
    auto iter = set_.find(i);
    bool in_set = iter != set_.end();
    if (in_set) {
      vtAssert(iter->in(val), "Value must exists in range");
    }
    return in_set;
  }

  void clear() {
    set_.clear();
    lb_ = ub_ = 0;
    hint_ = set_.end();
  }

  DomainT range() const { return ub_ - lb_; }
  DomainT lower() const { return lb_; }
  DomainT upper() const { return ub_; }
  bool    empty() const { return set_.size() == 0; }

  std::size_t size() const { return elms_; }
  std::size_t compressedSize() const { return set_.size(); }

  float compression() const {
    if (set_.size() > 0) {
      return static_cast<float>(elms_) / static_cast<float>(set_.size());
    } else {
      vtAssert(elms_ == 0, "Number of elements must be zero is set is empty");
      return 1.0f;
    }
  }

  void dumpState() const {
    debug_print(
      gen, node,
      "OrderedSet: bounds=[{},{}] size={}, compressedSize={}, compression={}\n",
      lb_, ub_, size(), compressedSize(), compression()
    );
    std::size_t c = 0;
    for (auto&& i : set_) {
      debug_print(gen, node, "\t interval {} : {}\n", c, i);
      c++;
    }
  }

private:

  IteratorType insertSet(IteratorType it, IntervalType&& i) {
    // Insert into the set
    auto ret = set_.emplace_hint(it,std::move(i));
    vtAssert(ret not_eq it,         "Should be a valid insert");
    vtAssert(ret not_eq set_.end(), "Must be valid insert---live iterator");

    // Fuse interval set elements that are now tangent after this insertion
    return hint_ = join(ret);
  }

  bool existsGlobal(DomainT const& val) const {
    return val >= lb_ and val <= ub_;
  }

  void eraseGlobal(DomainT const& val) {
    bool is_lower = lb_ == val;
    bool is_upper = ub_ == val;
    if (is_lower) {
      if (set_.size() > 0) {
        lb_ = set_.begin()->lower();
      } else {
        lb_ = 0;
      }
    }
    if (is_upper) {
      if (set_.size() > 0) {
        ub_ = std::prev(set_.end())->upper();
      } else {
        ub_ = 0;
      }
    }
    // Decrement count of non-compressed elements
    elms_--;
  }

  void insertGlobal(IntervalType const& i) {
    if (elms_ == 0) {
      lb_ = i.lower();
      ub_ = i.upper();
    } else {
      // Expand the global bounds in this interval set
      lb_ = std::min<DomainT>(lb_, i.lower());
      ub_ = std::max<DomainT>(ub_, i.upper());
    }
    // Increment count of non-compressed elements
    elms_++;
  }

  IteratorType join(IteratorType it) {
    auto it2 = joinLeft(it);
    return joinRight(it2);
  }

  IteratorType joinLeft(IteratorType it) {
    vtAssertExpr(it not_eq set_.end());
    if (it not_eq set_.begin()) {
      auto prev = std::prev(it);
      if (it->tangent(*prev) == PositionType::TangentLeft) {
        auto tmp = *prev;
        set_.erase(prev);
        auto& to_fuse = const_cast<IntervalType&>(*it);
        to_fuse.join(tmp, PositionType::TangentLeft);
      }
    }
    return it;
  }

  IteratorType joinRight(IteratorType it) {
    vtAssertExpr(it not_eq set_.end());
    auto next = std::next(it);
    if (it not_eq set_.end()) {
      if (it->tangent(*next) == PositionType::TangentRight) {
        auto tmp = *next;
        set_.erase(next);
        auto& to_fuse = const_cast<IntervalType&>(*it);
        to_fuse.join(tmp, PositionType::TangentRight);
      }
    }
    return it;
  }

public:

  //
  // This might be needed in the future for C++17 (std::make_reverse_iterator)
  //
  // using iterator_category = std::bidirectional_iterator_tag;
  // using difference_type   = DomainT;
  // using value_type        = DomainT;
  // using pointer           = DomainT*;
  // using reference         = DomainT&;

  template <typename Impl>
  struct IntervalSetIter :
    std::iterator<std::bidirectional_iterator_tag, DomainT, DomainT>
  {
    using Iter              = IntervalSetIter;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = DomainT;
    using value_type        = DomainT;
    using pointer           = DomainT;
    using reference         = DomainT;

    IntervalSetIter(Impl in_impl, std::size_t in_cur)
      : impl_(in_impl),
        cur_(in_cur)
    { }

    Iter& operator++() {
      if (impl_->lower() + cur_ < impl_->upper()) {
        cur_++;
      } else {
        cur_ = 0;
        impl_ = std::next(impl_);
      }
      return *this;
    }

    Iter& operator--() {
      if (cur_ == 0) {
        impl_ = std::prev(impl_);
        cur_ = impl_->width() - 1;
      } else {
        cur_--;
      }
      return *this;
    }

    Iter operator++(int) { Iter ret = *this; ++(*this); return ret; }
    Iter operator--(int) { Iter ret = *this; --(*this); return ret; }
    bool operator==(Iter i) const { return i.impl_ == impl_ and i.cur_ == cur_; }
    bool operator!=(Iter i) const { return !(*this == i); }
    pointer operator*() const { return impl_->get(cur_); }
    reference operator->() const { return impl_->get(cur_); }

  private:
    Impl impl_       = {};
    std::size_t cur_ = 0;
  };

  using ForwardIter  = IntervalSetIter<IteratorType>;
  using ReverseIter  = std::reverse_iterator<ForwardIter>;

  // Per-element iterators
  ForwardIter begin()  { return ForwardIter(set_.begin(),0); }
  ForwardIter end()    { return ForwardIter(set_.end(),0); }
  ReverseIter rbegin() { return ReverseIter(end()); }
  ReverseIter rend()   { return ReverseIter(begin()); }

  // Per-interval iterators
  IteratorType ibegin()  { return set_.begin(); }
  IteratorType iend()    { return set_.end(); }
  IteratorType irbegin() { return set_.rbegin(); }
  IteratorType irend()   { return set_.rend(); }

public:

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | lb_ | ub_;
    s | set_;

    //
    // @todo Use a more efficient unpack (like the following) so insertions are
    // O(1) since the elements are already in sorted order when they are
    // serialized
    //
    // typename OrderedSetType::size_type size = set_.size();
    // s | size;
    // if (s.isUnpacking()) {
    //   auto iter = set_.end();
    //   for (typename OrderedSetType::size_type = 0; i < size; i++) {
    //     typename OrderedSetType::value_type elm = {};
    //     s | elm;
    //     iter = set_.insert(iter,elm)
    //   }
    // } else {
    //   for (auto&& i : set_) {
    //     s | i;
    //   }
    // }

    if (s.isUnpacking()) {
      hint_ = set_.end();
    }
    s | elms_;
  }

private:
  // The lower bound for the entire set
  DomainT lb_         = 0;
  // The upper bound for the entire set
  DomainT ub_         = 0;
  // The set of all the interval ranges
  OrderedSetType set_ = {};
  // The previous insert iterator for quick lookup
  IteratorType hint_  = {};
  // Not required for correctness: count of non-compressed elements
  std::size_t elms_   = 0;
};


}}} /* end namespace vt::term::interval */

namespace vt {

template <typename DomainT>
using IntegralSet =
  term::interval::IntegralSetBase<
    DomainT, std::less<DomainT>, DomainT{}, std::allocator,
    term::interval::Interval, std::set
  >;

} /* end namespace vt */

#endif /*INCLUDED_VT_TERMINATION_INTERVAL_INTEGRAL_SET_H*/
