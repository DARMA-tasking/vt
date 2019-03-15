/*
//@HEADER
// ************************************************************************
//
//                          fetch_base.h
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
#include "vt/fetch/manager.fwd.h"
#include "vt/fetch/manager.h"

#include <vector>
#include <cstdlib>

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


template <typename T>
struct PtrTraits;

template <typename T>
struct PtrTraits<T*> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T, std::size_t N>
struct PtrTraits<T[N]> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits<T[]> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits<std::vector<T>> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = true;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T, std::size_t N>
struct PtrTraits<std::array<T, N>> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = true;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits {
  using BaseType = T;
  static constexpr bool arith = std::is_arithmetic<T>::value;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = PtrTraits<T>::dims;
};


template <typename T, typename U, typename Enable=void>
struct ConvTraits  {
   static constexpr bool conv = false;
};

template <typename T, typename U>
struct ConvTraits<
  T, U,
  typename std::enable_if_t<
    std::is_same<
      typename PtrTraits<T>::BaseType, typename PtrTraits<U>::BaseType
    >::value
    and
    PtrTraits<T>::dims == PtrTraits<U>::dims
  >
> {
  static constexpr bool conv = true;
};

struct IntNewTagType  { } InternalNewTag  { };
struct IntReadTagType { } InternalReadTag { };

struct Internal final {

  Internal() = delete;
  Internal(Internal const& i) = delete;
  Internal(std::nullptr_t) : refs_(nullptr), read_(nullptr)             { }
  Internal(IntNewTagType)  : refs_(new int), read_(new int), id_(ID())  { reset(); }
  Internal(Internal&& i)   : refs_(i.refs_), read_(i.read_), id_(i.id_) { inc(); }
  Internal(IntReadTagType, Internal const& i)
    : refs_(i.refs_), read_(i.read_), id_(i.id_)
  { incRead(); }
  Internal(Internal const& i, bool read)
    : refs_(i.refs_), read_(i.read_), id_(i.id_)
  { if (read) incRead(); else inc(); }

  Internal& operator=(std::nullptr_t) { clear(); return *this; }

  bool operator==(Internal const& i) const { return id_ == i.id_; }
  bool operator!=(Internal const& i) const { return id_ != i.id_; }
  bool operator==(std::nullptr_t) const    { return refs_ == nullptr; }
  bool operator!=(std::nullptr_t) const    { return refs_ != nullptr; }

  ~Internal() { clear(); }

  void reset() { *refs_ = 1; *read_ = 0; }
  void clear() { dec(); tryFree(); }
  void tryFree() {
    if (getRef() == 0) {
      vtAssertExpr(read_ != nullptr && *read_ == 0);
      free();
    }
  }
  void tryReadFree() {
    if (getRead() == 0) {
      readFree();
    }
  }
  void free() {
    clearDep();
    vtAssertExpr(refs_ != nullptr);
    vtAssertExpr(read_ != nullptr);
    delete refs_;
    delete read_;
    refs_ = nullptr;
    read_ = nullptr;
    theFetch()->freeFetch(id_);
  }
  void readFree() {
    vtAssertExpr(refs_ != nullptr);
    vtAssertExpr(read_ != nullptr);
    vtAssertExpr(*read_ == 0);
    theFetch()->freeFetchRead(id_);
  }

  void inc() { if (refs_) (*refs_)++; }
  void dec() { if (refs_) (*refs_)--; }
  int getRef() const { return refs_ ? *refs_ : -1; }

  void incRead() { if (read_) (*read_)++; inc(); }
  void decRead() { if (read_) (*read_)--; dec(); tryReadFree(); }
  int getRead() const { return read_ ? *read_ : -1; }

  bool valid() const { return id_ != no_fetch; }
  FetchType getID() const { return id_; }

  void dep(Internal* in, bool read) {
    if (dep_) { clearDep(); }
    dep_ = in;
    dep_read_ = read;
    vtAssertExpr(dep_ != nullptr);
    if (dep_read_) dep_->incRead(); else dep_->inc();
  }
  void clearDep() {
    if (dep_) {
      if (dep_read_) dep_->decRead(); else dep_->dec();
      dep_ = nullptr;
    }
  }

  static FetchType ID() { return theFetch()->newID(); }

private:
  int* refs_     = nullptr;
  int* read_     = nullptr;
  FetchType id_  = no_fetch;
  Internal* dep_ = nullptr;
  bool dep_read_ = false;
};

namespace trait {

enum FetchEnum : int8_t {
  Copy = 0x1,
  Ref  = 0x2,
  Read = 0x4
};

} /* end namespace trait */

template <typename std::underlying_type<trait::FetchEnum>::type flag>
struct Traits {
  enum : bool { Read = (flag & trait::FetchEnum::Read) != 0 };
  enum : bool { Copy = (flag & trait::FetchEnum::Copy) != 0 };
  enum : bool { Ref  = (flag & trait::FetchEnum::Ref) != 0  };

  using AddRead = Traits<flag | trait::FetchEnum::Read>;
};

using Default      = Traits<0x0>;
using Unique       = Traits<trait::FetchEnum::Copy>;
using Ref          = Traits<trait::FetchEnum::Ref>;
using ReadUnique   = Traits<trait::FetchEnum::Read | trait::FetchEnum::Copy>;
using Read         = Traits<trait::FetchEnum::Read | trait::FetchEnum::Ref>;

template <typename T, typename U, typename Enable=void>
struct Down  {
   static constexpr bool is = false;
};

template <typename T, typename U>
struct Down<
  T, U,
  typename std::enable_if_t<
    ((T::Read and U::Read) or (T::Read and not U::Read)) and
    (not T::Copy) and (not U::Copy)
  >
> {
  static constexpr bool is = true;
};

// template <typename T, typename Enable=void>
// struct IsCopy  {
//    static constexpr bool is = false;
// };

// template <typename T>
// struct IsCopy<
//   T,
//   typename std::enable_if_t<T::Copy>
// > {
//   static constexpr bool is = true;
// };


// template <typename T>
// struct Fetch<
//   T,
//   std::enable_if_t<
//     PtrTraits<T>::arith and PtrTraits<T>::dims >= 2 and not PtrTraits<T>::vec
//   >
// > {
//   using BaseType    = typename PtrTraits<T>::BaseType;
//   using BaseRefType = BaseType const&;

//   static constexpr auto nd = PtrTraits<T>::dims;

//   template <typename... Args>
//   BaseRefType get(Args&&... args) const {
//     static_assert(std::tuple_size<Args...>::value == nd, "Dims must match");
//   }

// private:
//   int64_t lo_[nd];
//   int64_t hi_[nd];
//   int64_t sd_[nd];
//   T* data_ = nullptr;
// };

struct FetchCopyTagType { } FetchCopyTag { };

template <typename T, typename Trait, typename Enable=void>
struct FetchImpl;


template <typename T, typename Trait>
struct FetchCtrl;


template <bool copy>
struct DisableCopyCons {
  DisableCopyCons() = default;
};

template <>
struct DisableCopyCons<true> {
  DisableCopyCons() = default;
  DisableCopyCons(DisableCopyCons const&) = delete;
};

/*
 * The base for FetchImpl
 */

template <typename Trait>
struct FetchImplBase : DisableCopyCons<Trait::Copy> {

  FetchImplBase() = default;
  FetchImplBase(FetchImplBase<Trait>&&) = default;

  template <typename Trait2>
  FetchImplBase(FetchImplBase<Trait2>&& in)
    : ctrl_(in.ctrl_,Trait2::Read)
  { }

  FetchImplBase(FetchImplBase<Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in), ctrl_(in.ctrl_,Trait::Read)
  { }

  template <typename Trait2>
  FetchImplBase(FetchCopyTagType, FetchImplBase<Trait2> const& in)
    : ctrl_(InternalNewTag)
  { }

  template <typename Trait2 = Trait>
  FetchImplBase<Trait2> copy() {
    return FetchImplBase<Trait2>(FetchCopyTag,*this);
  }

  FetchImplBase<typename Trait::AddRead> read() {
    return FetchImplBase<typename Trait::AddRead>(*this);
  }

  virtual ~FetchImplBase() = default;

private:
  Internal ctrl_ = nullptr;
};

/*
 * New FetchPayload
 */

struct PayloadNewTagType     { } PayloadNewTag     { };
struct PayloadNonownTagType  { } PayloadNonownTag  { };
struct PayloadRefTagType     { } PayloadRefTag     { };
struct PayloadCopyTagType    { } PayloadCopyTag    { };
struct PayloadMoveTagType    { } PayloadMoveTag    { };
struct PayloadUpdateTagType  { } PayloadUpdateTag  { };

template <typename T, typename Enable=void>
struct FetchPayload;

template <typename T>
struct FetchPayload<
  T,
  std::enable_if_t<
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and PtrTraits<T>::vec
  >
> {
  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  FetchPayload(std::nullptr_t)
    : span_(std::make_shared<SpanType>(SpanUnitializedTag))
  { }
  FetchPayload(PayloadNewTagType, int64_t len)
    : alloc_(std::make_unique<std::vector<ValueType>>(len)),
      span_(std::make_shared<SpanType>(&alloc_->at(0),len))
  { }
  FetchPayload(PayloadRefTagType, std::shared_ptr<SpanType> in_span)
    : span_(in_span)
  { }
  FetchPayload(PayloadRefTagType, FetchPayload<T> const& in)
    : span_(in.span_)
  { }
  FetchPayload(PayloadMoveTagType, FetchPayload<T>&& in)
    : alloc_(std::move(alloc_)), span_(std::move(in.span_))
  { }
  FetchPayload(PayloadCopyTagType, std::shared_ptr<SpanType> in_span)
    : alloc_(std::make_unique<std::vector<ValueType>>(in_span->size())),
      span_(std::make_shared<SpanType>(&alloc_->at(0),in_span->size()))
  {
    vtAssertExpr(in_span->init());
    span_->copySpan(*in_span);
  }
  template <typename U>
  FetchPayload(PayloadCopyTagType, FetchPayload<U> const& in)
    : FetchPayload(PayloadCopyTag, in.span_)
  { }

  void allocate(int64_t len) {
    alloc_ = std::make_unique<std::vector<ValueType>>(len);
    span_->update(&alloc_->at(0),len);
  }

  void update(std::vector<ValueType>&& in) {
    alloc_ = std::make_unique<std::vector<ValueType>>(std::move(in));
    span_->set(&alloc_->at(0),alloc_->size());
  }

  void update(std::vector<ValueType> const& in) {
    alloc_ = std::make_unique<std::vector<ValueType>>(in);
    span_->set(&alloc_->at(0),alloc_->size());
  }

  void update(std::vector<ValueType>* const ptr) {
    alloc_ = nullptr;
    span_->set(&ptr->at(0),ptr->size());
  }

  bool pending() const { return not span_->init(); }
  bool ready() const { return not pending(); }

  template <typename U, typename V>
  friend struct FetchCtrl;
  template <typename U, typename V>
  friend struct FetchPayload;

private:
  std::unique_ptr<std::vector<ValueType>> alloc_ = nullptr;
  std::shared_ptr<SpanType>               span_  = nullptr;
};

template <typename T>
struct FetchPayload<
  T,
  std::enable_if_t<
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec
  >
> {
  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  FetchPayload(std::nullptr_t)
    : span_(std::make_shared<SpanType>(SpanUnitializedTag))
  { }
  FetchPayload(PayloadNewTagType, int64_t len)
    : alloc_(std::make_unique<ValueType[]>(len)),
      span_(std::make_shared<SpanType>(alloc_.get(),len))
  { }
  FetchPayload(PayloadRefTagType, std::shared_ptr<SpanType> in_span)
    : span_(in_span)
  { }
  FetchPayload(PayloadRefTagType, FetchPayload<T> const& in)
    : span_(in.span_)
  { }
  FetchPayload(PayloadMoveTagType, FetchPayload<T>&& in)
    : alloc_(std::move(alloc_)), span_(std::move(in.span_))
  { }
  FetchPayload(PayloadCopyTagType, std::shared_ptr<SpanType> in_span)
    : alloc_(std::make_unique<ValueType[]>(in_span->size())),
      span_(std::make_shared<SpanType>(alloc_.get(),in_span->size()))
  {
    vtAssertExpr(in_span->init());
    span_->copySpan(*in_span);
  }
  template <typename U>
  FetchPayload(PayloadCopyTagType, FetchPayload<U> const& in)
    : FetchPayload(PayloadCopyTag, in.span_)
  { }

  void allocate(int64_t len) {
    alloc_ = std::make_unique<ValueType[]>(len);
    span_->update(alloc_.get(),len);
  }

  void update(ValueType* in_data, int64_t len) {
    span_->set(in_data,len);
  }

  bool pending() const { return not span_->init(); }
  bool ready() const { return not pending(); }

  template <typename U, typename V>
  friend struct FetchCtrl;
  template <typename U, typename V>
  friend struct FetchPayload;

private:
  std::unique_ptr<ValueType[]> alloc_ = nullptr;
  std::shared_ptr<SpanType>    span_  = nullptr;
};


/*
 * Main FetchCtrl specializations
 */

struct FetchPendingTagType     { } FetchPendingTag     { };
struct FetchFromPendingTagType { } FetchFromPendingTag { };




template <typename T, typename Trait>
struct FetchCtrl : DisableCopyCons<Trait::Copy> {

  using ValueType = typename PtrTraits<T>::BaseType;

  virtual ~FetchCtrl() = default;

  FetchCtrl() = default;
  FetchCtrl(FetchCtrl<T,Trait>&&) = default;
  FetchCtrl(FetchPendingTagType, std::string in_tag = "")
    : ctrl_(InternalNewTag), init_(true), tag_(in_tag)
  { }

  FetchCtrl(FetchCtrl<T,Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in),
      ctrl_(in.ctrl_,Trait::Read), init_(in.init_), tag_(in.tag_),
      payload_(FetchPayload<T>(PayloadRefTag, in.payload_))
  {
    vtAssertExpr(in.init_);
  }

  template <typename U, typename Trait2>
  FetchCtrl(FetchCopyTagType, FetchCtrl<U,Trait2> const& in)
    : ctrl_(InternalNewTag), init_(in.init_), tag_(in.tag_)
  {
    vtAssertExpr(not in.pending() and in.init_);
    payload_ = FetchPayload<T>(PayloadCopyTag, in.payload_);
  }

  template <typename U, typename Trait2>
  FetchCtrl(FetchFromPendingTagType, FetchCtrl<U,Trait2> const& in)
    : ctrl_(in.ctrl_), init_(in.init_), tag_(in.tag_), payload_(nullptr)
  {
    vtAssertExpr(in.pending());
  }

  template <typename Trait2>
  FetchCtrl(FetchCtrl<T,Trait2>&& in)
    : ctrl_(in.ctrl_,Trait2::Read), init_(in.init_),
      tag_(in.tag_), payload_(std::move(in.payload_))
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
  FetchCtrl(FetchCtrl<U,Trait2> const& in)
    : ctrl_(in.ctrl_,Trait::Read), init_(in.init_), tag_(in.tag_),
      payload_(FetchPayload<T>(PayloadRefTag, in.payload_))
  {
    vtAssertExpr(in.init_);
  }


  FetchType getID() const {
    vtAssertExpr(ctrl_.valid());
    return ctrl_.getID();
  }

  // @todo:
  //   - FetchCtrl constructors w/data immediately
  //   - enable_if guards for different data signatures

  /* ///////////////////////////////////////////////////////////////////////////
   * Three types of satisfaction could be implemented:
   *   1) With a pointer or reference to existing data
   *   2) With an existing FetchCtrl
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
   * Satisfy from an existing FetchCtrl
   */

  template <typename U = T, typename Trait2 = Trait>
  void satisfy(FetchCtrl<U,Trait2>& in) {
    vtAssertExpr(not in.pending() and in.ctrl_ not_eq nullptr);
    ctrl_.dep(&in.ctrl_, Trait2::Read);
    payload_ = FetchPayload<T>(PayloadRefTag, in.payload_);
    theFetch()->notifyReady(getID());
  }

  /*
   * The allocation version of satisfy which change the state by allocating new
   * data that satisfies the FetchCtrl
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

  /*
   * Trigger action when the data is ready if state is pending on data
   */
  void whenReady(ActionType action) {
    if (pending()) {
      theFetch()->whenReady(getID(),action);
    } else {
      action();
    }
  }


  // template <
  //   typename U = T,
  //   typename Trait2 = Trait,
  //   typename = typename std::enable_if_t<
  //     ConvTraits<T,U>::conv and Down<Trait,Trait2>::is
  //   >
  // >
  // FetchCtrl(FetchCtrl<U,Trait2> const& in)
  //   : SpanType(in), ctrl_(in.ctrl_,Trait2::Read)
  // { }


  // template <typename U, typename Trait2>
  // FetchCtrl(FetchCopyTagType, FetchCtrl<U,Trait2> const& in)
  //   : SpanType(SpanUnitializedTag),
  //     ctrl_(InternalNewTag),
  //     alloc_(std::make_unique<ValueType[]>(in.length()))
  // {
  //   SpanType::set(alloc_.get(),in.length());
  //   SpanType::copySpan(in);
  // }

  bool pending() const { return payload_.pending(); }
  bool ready() const { return not pending(); }

  template <typename U = T, typename Trait2 = Trait>
  FetchCtrl<U,Trait2> copy() {
    vtAssertExpr(not pending() and init_);
    return FetchCtrl<U,Trait2>(FetchCopyTag,*this);
  }

  FetchCtrl<T,typename Trait::AddRead> read() {
    vtAssertExpr(init_);
    return FetchCtrl<T,typename Trait::AddRead>(*this);
  }

  // void allocate(int64_t len) {
  //   alloc_ = std::make_unique<ValueType[]>(len);
  //   SpanType::set(alloc_.get(), len);
  // }

  template <typename U, typename V>
  friend struct FetchCtrl;

private:
  Internal ctrl_            = nullptr;
  bool init_                = false;
  std::string tag_          = "";
  FetchPayload<T> payload_  = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* Implement wrappers for creating and managing vt::fetch::Fetch<T> */

template <typename T, typename Trait = Default>
FetchCtrl<T,Trait> makePending(std::string tag = "") {
  return FetchCtrl<T,Trait>(FetchPendingTag, tag);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * Main FetchImpl specializations
 */

template <typename T, typename Trait>
struct FetchImpl<
  T, Trait,
  std::enable_if_t<
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec
  >
> : DisableCopyCons<Trait::Copy>, Span<typename PtrTraits<T>::BaseType> {

  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  static constexpr auto nd = PtrTraits<T>::dims;

  FetchImpl() : SpanType(SpanUnitializedTag) { }
  FetchImpl(FetchImpl<T,Trait>&&) = default;

  template <typename Trait2>
  FetchImpl(FetchImpl<T,Trait2>&& in)
    : SpanType(in), ctrl_(in.ctrl_,Trait2::Read), alloc_(std::move(in.alloc_))
  { }

  template <
    typename U = T,
    typename Trait2 = Trait,
    typename = typename std::enable_if_t<
      ConvTraits<T,U>::conv and Down<Trait,Trait2>::is
    >
  >
  FetchImpl(FetchImpl<U,Trait2> const& in)
    : SpanType(in), ctrl_(in.ctrl_,Trait2::Read)
  { }

  FetchImpl(FetchImpl<T,Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in), SpanType(in), ctrl_(in.ctrl_,Trait::Read)
  { }

  template <typename U, typename Trait2>
  FetchImpl(FetchCopyTagType, FetchImpl<U,Trait2> const& in)
    : SpanType(SpanUnitializedTag),
      ctrl_(InternalNewTag),
      alloc_(std::make_unique<ValueType[]>(in.length()))
  {
    SpanType::set(alloc_.get(),in.length());
    SpanType::copySpan(in);
  }

  virtual ~FetchImpl() = default;

  template <typename U = T, typename Trait2 = Trait>
  FetchImpl<U,Trait2> copy() {
    return FetchImpl<U,Trait2>(FetchCopyTag,*this);
  }

  FetchImpl<T,typename Trait::AddRead> read() {
    return FetchImpl<T,typename Trait::AddRead>(*this);
  }

  void allocate(int64_t len) {
    alloc_ = std::make_unique<ValueType[]>(len);
    SpanType::set(alloc_.get(), len);
  }

  int64_t length() const { return SpanType::size(); }
  bool owner() const { return alloc_ != nullptr; }
  std::size_t bytes() const { return length() * sizeof(ValueType); }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    SpanType::serialize(s);
  }

  template <typename U, typename V, typename W>
  friend struct FetchImpl;

private:
  Internal ctrl_ = nullptr;
  std::unique_ptr<ValueType[]> alloc_ = nullptr;
};

template <typename T, typename Trait>
struct FetchImpl<
  T, Trait,
  std::enable_if_t<
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and PtrTraits<T>::vec
  >
> : DisableCopyCons<Trait::Copy>, Span<typename PtrTraits<T>::BaseType> {

  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  FetchImpl() : SpanType(SpanUnitializedTag) { }
  FetchImpl(FetchImpl<T,Trait>&&) = default;


  template <typename Trait2>
  FetchImpl(FetchImpl<T,Trait2>&& in)
    : SpanType(in), ctrl_(in.ctrl_,Trait2::Read), alloc_(std::move(in.alloc_))
  { }



  template <
    typename Trait2,
    typename = typename std::enable_if_t<Down<Trait,Trait2>::is>
  >
  explicit FetchImpl(FetchImpl<T,Trait2> const& in)
    : ctrl_(in.ctrl_,Trait2::Read), SpanType(in)
  { }



  FetchImpl(FetchImpl<T,Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in),
      SpanType(in),
      ctrl_(in.ctrl_,Trait::Read)
  { }


  virtual ~FetchImpl() = default;


  template <typename U, typename Trait2>
  FetchImpl(FetchCopyTagType, FetchImpl<U,Trait2> const& in)
    : SpanType(SpanUnitializedTag),
      ctrl_(InternalNewTag),
      alloc_(std::make_unique<std::vector<ValueType>>(in.length()))
  {
    SpanType::set(&alloc_->at(0), alloc_->size());
    SpanType::copySpan(in);
  }

  int64_t length() const { return SpanType::size(); }

  template <typename U = T, typename Trait2 = Trait>
  FetchImpl<U,Trait2> copy() {
    return FetchImpl<U,Trait2>(FetchCopyTag,*this);
  }

  template <typename U, typename V, typename W>
  friend struct FetchImpl;


private:
  Internal ctrl_ = nullptr;
  std::unique_ptr<std::vector<ValueType>> alloc_ = nullptr;
};


template <typename T, typename Trait = Default>
using Fetch = FetchCtrl<T, Trait>;


}} /* end namespace vt::fetch */




namespace vt { namespace fetch {

// template <typename DataT, typename Enable=void>
// struct FetchTraits;

// template <typename DataT>
// struct FetchTraits<
//   DataT,
//   typename std::enable_if_t<
//     std::is_same<DataT, std::vector<typename DataT::value_type>>::value
//   >
// > : Span<typename DataT::value_type> {

//   using ValueType = typename DataT::value_type;
//   using SpanType  = Span<ValueType>;

//   explicit FetchTraits(std::vector<ValueType>& in)
//     : SpanType(&in[0],in.size()),
//       ptr_(&in),
//       owning_(false)
//   { }
//   FetchTraits() : Span<ValueType>(SpanUnitializedTag) { }

//   void set(std::vector<ValueType>& in) {
//     auto const& init = SpanType::init();
//     if (init) {
//       vtAssert(in.size() == SpanType::size(), "Sizes must be identical");
//       std::memcpy(SpanType::get(), &in[0], in.size()*sizeof(ValueType));
//     } else {
//       ptr_ = &in;
//       SpanType::set(&in[0], in.size());
//     }
//   }

//   void set(std::vector<ValueType>&& in) {
//     auto const& init = SpanType::init();
//     if (init) {
//       *ptr_ = std::move(in);
//     } else {
//       ptr_ = new std::vector<ValueType>(std::move(in));
//       owning_ = true;
//     }
//     SpanType::set(&(*ptr_)[0], ptr_->size());
//   }

//   std::size_t totalBytes() const {
//     return SpanType::init() ? SpanType::size() * sizeof(ValueType) : 0;
//   }

//   std::vector<ValueType> const& operator*() { return *ptr_; }

//   virtual ~FetchTraits() {
//     if (ptr_ and owning_) {
//       delete ptr_;
//       ptr_ = nullptr;
//       owning_ = false;
//     }
//   }

//   template <typename SerializerT>
//   void serialize(SerializerT& s) {
//     SpanType::serialize(s);
//   }

// private:
//   std::vector<ValueType>* ptr_ = nullptr;
//   bool owning_ = false;
// };


// template <typename T>
// struct FetchTraits<
//   T,
//   typename std::enable_if_t<std::is_arithmetic<T>::value>
// > : Span<T> {
//   using ValueType = T;
//   using SpanType  = Span<ValueType>;

//   FetchTraits(T* in, int64_t in_len)
//     : SpanType(in,in_len)
//   { }
//   FetchTraits() : SpanType(SpanUnitializedTag) { }

//   void set(T* in, int64_t len) {
//     auto const& init = SpanType::init();
//     if (init) {
//       std::memcpy(SpanType::get(), in, len*sizeof(T));
//     } else {
//       SpanType::set(in, len);
//     }
//   }

//   std::size_t totalBytes() const {
//     return SpanType::init() ? SpanType::size() * sizeof(T) : 0;
//   }

//   template <typename SerializerT>
//   void serialize(SerializerT& s) {
//     SpanType::serialize(s);
//   }
// };

// template <typename T>
// using Fetch = FetchTraits<T>;

// template <typename T>
// struct Fetch : FetchTraits<T> {
//   Fetch() = delete;
//   Fetch(Fetch const&) = default;
//   Fetch(Fetch&&) = default;
//   Fetch& operator=(Fetch const&) = default;
//   virtual ~Fetch() = default;
// };

}} /* end namespace vt::fetch */

namespace vt {

// template <typename T>
// using Fetch = fetch::Fetch<T>;

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H*/
