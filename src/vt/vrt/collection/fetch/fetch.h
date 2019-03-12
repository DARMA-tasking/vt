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
#include "vt/vrt/collection/fetch/fetch_base.h"
#include "vt/vrt/collection/fetch/span.h"

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
  static constexpr bool vector = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T, std::size_t N>
struct PtrTraits<T[N]> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vector = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits<T[]> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vector = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits<std::vector<T>> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vector = true;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T, std::size_t N>
struct PtrTraits<std::array<T, N>> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vector = false;
  static constexpr bool array = true;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits {
  using BaseType = T;
  static constexpr bool arith = std::is_arithmetic<T>::value;
  static constexpr bool vector = false;
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

struct InternalNewTagType { } InternalNewTag { };

struct Internal final {

  Internal() = delete;

  Internal(std::nullptr_t)     : refs_(nullptr) { }
  Internal(InternalNewTagType) : refs_(new int) { *refs_ = 1; }
  Internal(Internal const& i)  : refs_(i.refs_) { incRef();   }
  Internal(Internal&& i)       : refs_(i.refs_) { incRef();   }

  Internal& operator=(std::nullptr_t) { clear(); return *this; }

  bool operator==(Internal const& i) const { return refs_ == i.refs_; }
  bool operator!=(Internal const& i) const { return refs_ != i.refs_; }
  bool operator==(std::nullptr_t) const    { return refs_ == nullptr; }
  bool operator!=(std::nullptr_t) const    { return refs_ != nullptr; }

  ~Internal() { clear(); }

  void clear() { decRef(); tryFree(); }
  void tryFree() { if (getRef() == 0) { free(); } }
  void free() {
    vtAssertExpr(refs_ != nullptr);
    delete refs_;
    refs_ = nullptr;
  }

  //void mkOwner() { refs_ = new int; *refs_ = 1; }
  void incRef() { if (refs_) (*refs_)++; }
  void decRef() { if (refs_) (*refs_)--; }
  int getRef() const { return refs_ ? *refs_ : -1; }

protected:
  int* refs_ = nullptr;
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
//     PtrTraits<T>::arith and PtrTraits<T>::dims >= 2 and not PtrTraits<T>::vector
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

template <bool copy>
struct DisableCopyCons {
  DisableCopyCons() = default;
};

template <>
struct DisableCopyCons<true> {
  DisableCopyCons() = default;
  DisableCopyCons(DisableCopyCons const&) = delete;
};

template <typename T, typename Trait>
struct FetchImpl<
  T, Trait,
  std::enable_if_t<
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and not PtrTraits<T>::vector
  >
> : DisableCopyCons<Trait::Copy>, Span<typename PtrTraits<T>::BaseType> {

  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  static constexpr auto nd = PtrTraits<T>::dims;

  FetchImpl() : SpanType(SpanUnitializedTag) { }
  FetchImpl(FetchImpl<T,Trait>&&) = default;

  template <typename Trait2>
  FetchImpl(FetchImpl<T,Trait2>&& in)
    : SpanType(in), ctrl_(in.ctrl_), alloc_(std::move(in.alloc_))
  { }

  template <
    typename U = T,
    typename Trait2 = Trait,
    typename = typename std::enable_if_t<
      ConvTraits<T,U>::conv and Down<Trait,Trait2>::is
    >
  >
  FetchImpl(FetchImpl<U,Trait2> const& in)
    : SpanType(in), ctrl_(in.ctrl_)
  { }

  FetchImpl(FetchImpl<T,Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in), SpanType(in), ctrl_(in.ctrl_)
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
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and PtrTraits<T>::vector
  >
> : DisableCopyCons<Trait::Copy>, Span<typename PtrTraits<T>::BaseType> {

  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  FetchImpl() : SpanType(SpanUnitializedTag) { }
  FetchImpl(FetchImpl<T,Trait>&&) = default;


  template <typename Trait2>
  FetchImpl(FetchImpl<T,Trait2>&& in)
    : SpanType(in), ctrl_(in.ctrl_), alloc_(std::move(in.alloc_))
  { }



  template <
    typename Trait2,
    typename = typename std::enable_if_t<Down<Trait,Trait2>::is>
  >
  explicit FetchImpl(FetchImpl<T,Trait2> const& in)
    : ctrl_(in.ctrl_), SpanType(in)
  { }



  FetchImpl(FetchImpl<T,Trait> const& in)
    : DisableCopyCons<Trait::Copy>(in), SpanType(in), ctrl_(in.ctrl_)
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
using Fetch = FetchImpl<T, Trait>;


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
