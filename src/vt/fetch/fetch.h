/*
//@HEADER
// ************************************************************************
//
//                            fetch.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H
#define INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H

#include "vt/config.h"
#include "vt/fetch/fetch_base.h"
#include "vt/fetch/span.h"
#include "vt/fetch/ctrl.h"
#include "vt/fetch/payload/all.h"
#include "vt/fetch/traits/ptr.h"
#include "vt/fetch/traits/mode.h"
#include "vt/fetch/traits/conversion.h"
#include "vt/fetch/traits/disable_copy_cons.h"

#include <vector>
#include <cstdlib>
#include <memory>
#include <type_traits>

namespace vt { namespace fetch {

/*
 * vt::Fetch<Kokkos::View<int*>>
 * vt::Fetch<int*>
 * vt::Fetch<std::vector<int>>
 * vt::Fetch<std::map<int,double>>
 * vt::Fetch<std::set<Blob>>
 * vt::Fetch<double[20]>
 * vt::Fetch<int>
 */



template <typename T, typename Trait = Default>
struct Fetch;

static struct FetchCopyTagType        { } FetchCopyTag        { };
static struct FetchPendingTagType     { } FetchPendingTag     { };

template <typename T, typename Trait>
struct Fetch : DisableCopyCons<Trait::Copy> {

  using ValueType = typename PtrTraits<T>::BaseType;

  static_assert(PtrTraits<T>::dims < 2, "Dimensions beyond 1 not supported");

  virtual ~Fetch() = default;

  Fetch() = default;
  Fetch(Fetch<T,Trait>&&) = default;
  Fetch(FetchPendingTagType, std::string in_tag = "")
    : ctrl_(CtrlNewTag), init_(true)
  { }

  Fetch(Fetch<T,Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in),
      ctrl_(in.ctrl_,Trait::Read), init_(in.init_),
      payload_(FetchPayload<T>(PayloadRefTag, in.payload_))
  {
    vtAssertExpr(in.init_);
  }

  template <typename U, typename Trait2>
  Fetch(FetchCopyTagType, Fetch<U,Trait2> const& in)
    : ctrl_(CtrlNewTag), init_(in.init_)
  {
    vtAssertExpr(not in.pending() and in.init_);
    payload_ = FetchPayload<T>(PayloadCopyTag, in.payload_);
  }

  template <typename Trait2>
  Fetch(Fetch<T,Trait2>&& in)
    : ctrl_(in.ctrl_,Trait2::Read), init_(in.init_),
      payload_(std::move(in.payload_))
  { }

  /*
   * Convertible copy constructor with reference semantics across types, where U
   * is convertible to T and there is the down relationship across usage levels
   */
  template <
    typename U = T,
    typename Trait2 = Trait,
    typename = typename std::enable_if_t<
      ConvTraits<T,U>::conv and Down<Trait,Trait2>::is
    >
  >
  Fetch(Fetch<U,Trait2> const& in)
    : ctrl_(in.ctrl_,Trait::Read), init_(in.init_),
      payload_(FetchPayload<T>(PayloadRefTag, in.payload_))
  {
    vtAssertExpr(in.init_);
  }

  /*
   * Copy-assignment operator has by-default reference semantics as expected
   */
  Fetch<T,Trait>& operator=(Fetch<T,Trait> const& in) {
    if (this != &in) {
      ctrl_ = Ctrl(in.ctrl_,Trait::Read);
      init_ = in.init_;
      payload_ = FetchPayload<T>(PayloadRefTag, in.payload_);
    }
    return *this;
  }

  FetchType getID() const {
    vtAssertExpr(ctrl_.valid());
    return ctrl_.getID();
  }

  // @todo:
  //   - Fetch constructors w/data immediately
  //   - enable_if guards for different data signatures

  /* ///////////////////////////////////////////////////////////////////////////
   * Three types of satisfaction could be implemented:
   *   1) With a pointer or reference to existing data
   *   2) With an existing Fetch
   *   3) With a new allocation
   * ///////////////////////////////////////////////////////////////////////////
   */

  /*
   * Raw 1D pointer satisfy overloads. std::vector<ValueType> is one-way down
   * convertible to a 1D pointer
   */
  template <
    typename U = void,
    typename = typename std::enable_if<
      PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec, U
    >::type
  >
  void satisfy(ValueType* ptr, int64_t len) {
    payload_.update(ptr, len);
    theFetch()->notifyReady(getID());
  }

  /*
   * Vector satisfy overloads
   */

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and PtrTraits<T>::vec, U
    >::type
  >
  void satisfy(std::vector<ValueType>&& in) {
    payload_.update(std::move(in));
    theFetch()->notifyReady(getID());
  }

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and PtrTraits<T>::vec, U
    >::type
  >
  void satisfy(std::vector<ValueType>* const in) {
    payload_.update(in);
    theFetch()->notifyReady(getID());
  }

  /*
   * Satisfy from an existing Fetch
   */

  template <typename U = T, typename Trait2 = Trait>
  void satisfy(Fetch<U,Trait2>& in) {
    vtAssertExpr(not in.pending() and in.ctrl_ not_eq nullptr);
    ctrl_.dep(&in.ctrl_, Trait2::Read);
    payload_.updateInternal(in.payload_);
    theFetch()->notifyReady(getID());
  }

  /*
   * The allocation version of satisfy which change the state by allocating new
   * data that satisfies the Fetch
   */

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::arith and PtrTraits<T>::dims == 1, U
    >::type
  >
  void satisfyAlloc(int64_t len) {
    payload_.allocate(len);
    theFetch()->notifyReady(getID());
  }

  template <
    typename... Args,
    typename U = void,
    typename   = typename std::enable_if<
      not (PtrTraits<T>::arith and PtrTraits<T>::dims == 1), U
    >::type
  >
  void satisfyAlloc(Args&&... args) {
    payload_.allocate(std::forward<Args>(args)...);
    theFetch()->notifyReady(getID());
  }

  /*
   * Trigger action when the data is ready if state is pending on data
   */
  void onReady(ActionType action) {
    if (pending()) {
      theFetch()->whenReady(getID(),action);
    } else {
      action();
    }
  }

  /*
   * Trigger action when all outstanding reads are complete
   */
  void afterRead(ActionType action) {
    if (ctrl_.getRead() == 0) {
      action();
    } else {
      theFetch()->whenFinishRead(getID(),action);
    }
  }

  bool pending() const { return payload_.pending(); }
  bool ready() const { return not pending(); }
  bool hasReads() const { return ctrl_.getRead() > 0; }

  template <typename U = T, typename Trait2 = Trait>
  Fetch<U,Trait2> copy() {
    vtAssertExpr(not pending() and init_);
    return Fetch<U,Trait2>(FetchCopyTag,*this);
  }

  Fetch<T,typename Trait::AddRead> read() {
    vtAssertExpr(init_);
    return Fetch<T,typename Trait::AddRead>(*this);
  }

  Fetch<T,Trait> ref() {
    vtAssertExpr(init_);
    return Fetch<T,Trait>(*this);
  }

  /*
   * Interface for actually accesses the data, type of reference/pointer
   * conditionalized on whether it's a Trait::Read or not
   */

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::arith and PtrTraits<T>::dims == 1, U
    >::type
  >
  ValueType* getRaw() const { return payload_.getPtr(); }

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec, U
    >::type
  >
  ValueType* get() const { return payload_.getPtr(); }

  template <
    typename U = void,
    typename   = typename std::enable_if<
      (not (PtrTraits<T>::arith and PtrTraits<T>::dims == 1) or PtrTraits<T>::vec)
      and Trait::Read, U
    >::type
  >
  T const& get() const { return *payload_.get(); }

  template <
    typename U = void,
    typename   = typename std::enable_if<
      (not (PtrTraits<T>::arith and PtrTraits<T>::dims == 1) or PtrTraits<T>::vec)
      and not Trait::Read, U
    >::type
  >
  T& get() const { return *payload_.get(); }

  /*
   * Syntactic sugar for getting data
   */

  template <
    typename U = void,
    typename   = typename std::enable_if<Trait::Read, U>::type
  >
  T const& operator*() { return *payload_.get(); }

  template <
    typename U = void,
    typename   = typename std::enable_if<Trait::Read, U>::type
  >
  T const* operator->() { return payload_.get(); }

  template <
    typename U = void,
    typename   = typename std::enable_if<not Trait::Read, U>::type
  >
  T& operator*() { return *payload_.get(); }

  template <
    typename U = void,
    typename   = typename std::enable_if<not Trait::Read, U>::type
  >
  T* operator->() { return payload_.get(); }

  /*
   * Syntactic sugar for forwarding size, paren, and bracket operators
   */

  template <
    typename... Args,
    typename U = void,
    typename   = typename std::enable_if<
      not (PtrTraits<T>::dims == 1) or PtrTraits<T>::vec, U
    >::type
  >
  decltype(auto) size(Args&&... args) {
    return payload_.get()->size(std::forward<Args>(args)...);
  }

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec, U
    >::type
  >
  int64_t size() {
    return payload_.size();
  }

  template <
    typename... Args,
    typename U = void,
    typename   = typename std::enable_if<
      not (PtrTraits<T>::dims == 1) or PtrTraits<T>::vec, U
    >::type
  >
  decltype(auto) operator[](Args&&... args) {
    return payload_.get()->operator[](std::forward<Args>(args)...);
  }

  template <
    typename U = void,
    typename   = typename std::enable_if<
      PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec, U
    >::type
  >
  ValueType& operator[](int64_t val) {
    return *(payload_.get() + val);
  }

  /*
   * Overload that may use useful for Kokkos::Views
   */
  template <typename... Args>
  decltype(auto) operator()(Args&&... args) {
    return payload_.get()->operator()(std::forward<Args>(args)...);
  }

  template <typename U, typename V>
  friend struct Fetch;

private:
  Ctrl ctrl_                = nullptr;
  bool init_                = false;
  FetchPayload<T> payload_  = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* Implement wrappers for creating and managing vt::fetch::Fetch<T> */

template <typename T, typename Trait = Default>
Fetch<T,Trait> makePending(std::string tag = "") {
  return Fetch<T,Trait>(FetchPendingTag, tag);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

}} /* end namespace vt::fetch */

namespace vt {

template <typename T, typename Trait = fetch::Default>
using Fetch = fetch::Fetch<T, Trait>;

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H*/
