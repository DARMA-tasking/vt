/*
//@HEADER
// *****************************************************************************
//
//                                   union.h
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

#if !defined INCLUDED_VT_UTILS_ADT_UNION_H
#define INCLUDED_VT_UTILS_ADT_UNION_H

#include "vt/config.h"

namespace vt { namespace util { namespace adt {
namespace detail {

///////////////////////////////////////////////////////////////////////////////

/**
 * \internal \struct Sizer
 *
 * \brief Union of char[sizeof(U)] all types (T, Ts...) which is never
 * constructed but used to calculate the appropriate size of the
 * \c AlignedCharUnion to hold any of these types.
 */
template <typename T, typename... Ts>
union Sizer {
  char _cur[sizeof(T)];
  Sizer<Ts...> _rest;
};

template <typename T>
union Sizer<T> {
  char _cur[sizeof(T)];
};

///////////////////////////////////////////////////////////////////////////////

/**
 * \internal \struct Aligner
 *
 * \brief Contains all the types (T, Ts...) to determine the max alignment
 * required for them for the alignas in the \c AlignedCharUnion
 */
template <typename T, typename... Ts>
struct Aligner {
  Aligner() = delete;
  T _cur;
  Aligner<Ts...> _rest;
};

template <typename T>
struct Aligner<T> {
  Aligner() = delete;
  T _cur;
};

///////////////////////////////////////////////////////////////////////////////

/// Used to assert that \c T is included in \c Us
template <typename T, typename... Us>
struct MustBe;

template <typename T, typename U, typename... Us>
struct MustBe<T, U, Us...> {
  static constexpr bool const is_same =
    std::is_same<T,U>::value or MustBe<T,Us...>::is_same;
};

template <typename T>
struct MustBe<T> {
  static constexpr bool const is_same = false;
};

///////////////////////////////////////////////////////////////////////////////
// Turn a pack into a \c char and indicates order in the pack for selection

template <typename... Us>
struct GetPlace {
  static constexpr uint8_t const value = sizeof...(Us) + 1;
};

///////////////////////////////////////////////////////////////////////////////
/// Find the place of a given \c T inside a pack \c U, Us...

template <typename T, typename U, typename... Us>
struct Which;

template <typename T, typename U, typename Enable, typename... Us>
struct WhichImpl;

template <typename T, typename U, typename... Us>
struct WhichImpl<T, U, typename std::enable_if_t<std::is_same<T,U>::value>, Us...> {
  static constexpr uint8_t const value = sizeof...(Us) + 1;
};

template <typename T, typename U, typename... Us>
struct WhichImpl<T, U, typename std::enable_if_t<not std::is_same<T,U>::value>, Us...> {
  static constexpr uint8_t const value = Which<T, Us...>::value;
};

template <typename T, typename U, typename... Us>
struct Which : WhichImpl<T, U, void, Us...> { };

///////////////////////////////////////////////////////////////////////////////

/// Automatically deallocate based on active element
template <typename T, typename... Ts>
struct Deallocate {
  template <typename U>
  static void apply(uint8_t which, U* u) {
    if (which == GetPlace<Ts...>::value) {
      u->template deallocateAs<T>();
    } else {
      Deallocate<Ts...>::apply(which, u);
    }
  }
};

template <typename T>
struct Deallocate<T> {
  template <typename U>
  static void apply(uint8_t which, U* u) {
    if (which == static_cast<uint8_t>(1)) {
      u->template deallocateAs<T>();
    } else {
      vtAssert(false, "Invalid type; can not deallocate");
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

/// Automatically invoke the right copy constructor
template <typename T, typename... Ts>
struct Copy {
  static void apply(uint8_t which, char const* from, char* to) {
    if (which == GetPlace<Ts...>::value) {
      new (to) T{*reinterpret_cast<T const*>(from)};
    } else {
      Copy<Ts...>::apply(which, from, to);
    }
  }
};

template <typename T>
struct Copy<T> {
  static void apply(uint8_t which, char const* from, char* to) {
    if (which == static_cast<uint8_t>(1)) {
      new (to) T{*reinterpret_cast<T const*>(from)};
    } else {
      vtAssert(false, "Invalid type; can not copy");
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

/// Automatically invoke the right move constructor
template <typename T, typename... Ts>
struct Move {
  static void apply(uint8_t which, char* from, char* to) {
    if (which == GetPlace<Ts...>::value) {
      new (reinterpret_cast<T*>(to)) T{std::move(*reinterpret_cast<T*>(from))};
    } else {
      Move<Ts...>::apply(which, from, to);
    }
  }
};

template <typename T>
struct Move<T> {
  static void apply(uint8_t which, char* from, char* to) {
    if (which == static_cast<uint8_t>(1)) {
      new (reinterpret_cast<T*>(to)) T{std::move(*reinterpret_cast<T*>(from))};
    } else {
      vtAssert(false, "Invalid type; can not move");
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

/// Automatically invoke the right serializer
template <typename T, typename... Ts>
struct Serialize {
  template <typename U, typename SerializerT>
  static void apply(uint8_t which, U* u, SerializerT& s) {
    if (which == GetPlace<Ts...>::value) {
      u->template serializeAs<T>(s);
    } else {
      Serialize<Ts...>::apply(which, u, s);
    }
  }
};

template <typename T>
struct Serialize<T> {
  template <typename U, typename SerializerT>
  static void apply(uint8_t which, U* u, SerializerT& s) {
    if (which == static_cast<uint8_t>(1)) {
      u->template serializeAs<T>(s);
    } else {
      vtAssert(false, "Invalid type; can not serialize");
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

/// Automatically invoke the underlying comparison
template <typename T, typename... Ts>
struct Compare {
  template <typename U>
  static bool apply(uint8_t which, U const* u1, U const* u2) {
    if (which == GetPlace<Ts...>::value) {
      return u1->template compareAs<T>(u2);
    } else {
      return Compare<Ts...>::apply(which, u1, u2);
    }
  }
};

template <typename T>
struct Compare<T> {
  template <typename U>
  static bool apply(uint8_t which, U const* u1, U const* u2) {
    if (which == static_cast<uint8_t>(1)) {
      return u1->template compareAs<T>(u2);
    } else {
      vtAssert(false, "Invalid type; can not compare");
      return false;
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

/// Automatically invoke the right hash
template <typename T, typename... Ts>
struct Hash {
  template <typename U>
  static std::size_t apply(uint8_t which, U const* u1) {
    if (which == GetPlace<Ts...>::value) {
      return u1->template hashAs<T>();
    } else {
      return Hash<Ts...>::apply(which, u1);
    }
  }
};

template <typename T>
struct Hash<T> {
  template <typename U>
  static std::size_t apply(uint8_t which, U const* u1) {
    if (which == static_cast<uint8_t>(1)) {
      return u1->template hashAs<T>();
    } else {
      vtAssert(false, "Invalid type; can not compare");
      return false;
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename... Ts>
struct IsTriviallyDestructible {
  static constexpr bool const value =
    std::is_trivially_destructible<T>::value and IsTriviallyDestructible<Ts...>::value;
};

template <typename T>
struct IsTriviallyDestructible<T> {
  static constexpr bool const value = std::is_trivially_destructible<T>::value;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename... Ts>
struct IsTriviallyCopyable {
  static constexpr bool const value =
    std::is_trivially_copyable<T>::value and IsTriviallyCopyable<Ts...>::value;
};

template <typename T>
struct IsTriviallyCopyable<T> {
  static constexpr bool const value = std::is_trivially_copyable<T>::value;
};

///////////////////////////////////////////////////////////////////////////////

template <typename... Ts>
struct AllUnique;

template <typename U, typename T, typename... Ts>
struct AllUnique<U, T, Ts...>
  : AllUnique<U, T>, AllUnique<U, Ts...>, AllUnique<T, Ts...> { };

template <typename U, typename T>
struct AllUnique<U, T> {
  static_assert(not std::is_same<U, T>::value, "Types must be unique");
};

///////////////////////////////////////////////////////////////////////////////
/// Statically dispatch switch statement on typed functor

template <typename T, typename... Ts>
struct Switch {
  template <typename U, typename Functor>
  static auto apply(uint8_t which, U* u, Functor& fn) {
    if (which == GetPlace<Ts...>::value) {
      return fn.template apply<T>(*u);
    } else {
      return Switch<Ts...>::template apply(which, u, fn);
    }
  }
};

template <typename T>
struct Switch<T> {
  template <typename U, typename Functor>
  static auto apply(uint8_t which, U* u, Functor& fn) {
    if (which == static_cast<uint8_t>(1)) {
      return fn.template apply<T>(*u);
    } else {
      return fn.template apply<void>(*u);
    }
  }
};

} /* end namespace detail */

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename... Ts>
struct UnionBase {

  UnionBase() = default;
  explicit UnionBase(uint8_t in_which)
    : which_(in_which)
  { }

  /**
   * \brief Get the char* to underlying bytes
   */
  char* getUnsafeRawBytes() { return static_cast<char*>(this->data_); }

protected:
  alignas(detail::Aligner<T,Ts...>) char data_[sizeof(detail::Sizer<T,Ts...>)];
  uint8_t which_ = 0;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename Enable, typename... Ts>
struct UnionDestroy;

template <typename T, typename... Ts>
struct UnionDestroy<
  T,
  typename std::enable_if_t<detail::IsTriviallyDestructible<T, Ts...>::value>,
  Ts...
> : UnionBase<T, Ts...> {

  UnionDestroy() = default;
  explicit UnionDestroy(uint8_t in_which)
    : UnionBase<T, Ts...>(in_which)
  { }

  /**
   * \brief Reset the union, calling the appropriate destructor if one variant
   * is active.
   */
  void reset() {
    this->which_ = 0;
  }
};

template <typename T, typename... Ts>
struct UnionDestroy<
  T,
  typename std::enable_if_t<not detail::IsTriviallyDestructible<T, Ts...>::value>,
  Ts...
> : UnionBase<T, Ts...> {

  UnionDestroy() = default;
  explicit UnionDestroy(uint8_t in_which)
    : UnionBase<T, Ts...>(in_which)
  { }

  /**
   * \brief Reset the union, calling the appropriate destructor if one variant
   * is active.
   */
  void reset() {
    if (this->which_ != 0) {
      detail::Deallocate<T, Ts...>::apply(this->which_, this);
      this->which_ = 0;
    }
  }

  /**
   * \brief Deallocate as a certain type \c U
   */
  template <typename U>
  void deallocateAs() {
    vtAssert(
      (this->which_ == detail::Which<U, T, Ts...>::value),
      "Deallocating as wrong type"
    );
    reinterpret_cast<U*>(this->getUnsafeRawBytes())->~U();
    this->which_ = 0;
  }

  ~UnionDestroy() { reset(); }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename Enable, typename... Ts>
struct UnionCopy;

template <typename T, typename... Ts>
struct UnionCopy<
  T,
  typename std::enable_if_t<detail::IsTriviallyCopyable<T, Ts...>::value>,
  Ts...
> : UnionDestroy<T, void, Ts...> {

  UnionCopy() = default;

};

template <typename T, typename... Ts>
struct UnionCopy<
  T,
  typename std::enable_if_t<not detail::IsTriviallyCopyable<T, Ts...>::value>,
  Ts...
> : UnionDestroy<T, void, Ts...> {

  UnionCopy() = default;

  UnionCopy(UnionCopy&& other) {
    this->which_ = other.which_;
    if (this->which_ != 0) {
      detail::Move<T, Ts...>::apply(this->which_, other.data_, this->data_);
    }
  }

  UnionCopy(UnionCopy const& other) {
    this->which_ = other.which_;
    if (this->which_ != 0) {
      detail::Copy<T, Ts...>::apply(this->which_, other.data_, this->data_);
    }
  }

  UnionCopy& operator=(UnionCopy const& other) {
    this->reset();
    this->which_ = other.which_;
    if (this->which_ != 0) {
      detail::Copy<T, Ts...>::apply(this->which_, other.data_, this->data_);
    }
    return *this;
  }

  UnionCopy& operator=(UnionCopy&& other) {
    this->reset();
    this->which_ = other.which_;
    if (this->which_ != 0) {
      detail::Move<T, Ts...>::apply(this->which_, other.data_, this->data_);
    }
    return *this;
  }
};

///////////////////////////////////////////////////////////////////////////////

/**
 * \struct AlignedCharUnion
 *
 * \brief An aligned type-safe union that remembers its last type and runtime
 * checks to ensure correctness.
 *
 * Example of use:
 *
 * \code{.cpp}
 *    struct Test1 {
 *      Test1(int in_a) : a_(in_a) {}
 *      int a_;
 *    };
 *
 *    AlignedCharUnion<int, float, Test1> x;
 *    x.init<Test1>(100);
 *    x.get<Test1>.a_ = 10;
 *    x.reset();
 *    x.init<int>();
 *    x.get<int> = 10;
 * \endcode
 */
template <typename T, typename... Ts>
struct AlignedCharUnion : UnionCopy<T, void, Ts...> {

  AlignedCharUnion() = default;

  std::size_t hash() const {
    if (this->which_ != 0) {
      return detail::Hash<T, Ts...>::apply(this->which_, this);
    } else {
      return 0;
    }
  }

  bool operator==(AlignedCharUnion const& other) const {
    if (this->which_ != other.which_) {
      return false;
    }
    if (this->which_ == 0) {
      return true;
    }
    return detail::Compare<T, Ts...>::apply(this->which_, this, &other);
  }

  /**
   * \brief Initialize as \c U with arguments to constructor \c Args
   *
   * \param[in] args constructor arguments
   */
  template <typename U, typename... Args>
  void init(Args&&... args) {
    staticAssertCorrectness<U>();
    vtAssert(this->which_ == 0, "Must be uninitialized");
    this->which_ = detail::Which<U, T, Ts...>::value;
    auto t = getUnsafe<U>();
    new (t) U{std::forward<Args>(args)...};
  }

  /**
   * \brief Check if union has active type \c U
   *
   * \return whether \c U is active
   */
  template <typename U>
  bool is() const {
    staticAssertCorrectness<U>();
    return this->which_ == detail::Which<U, T, Ts...>::value;
  }

  /**
   * \brief Get a reference as a certain type \c U
   */
  template <typename U>
  U& get() {
    staticAssertCorrectness<U>();
    return *getSafe<U>();
  }

  /**
   * \brief Get a const reference as a certain type \c U
   */
  template <typename U>
  U const& get() const {
    staticAssertCorrectness<U>();
    return *getSafe<U>();
  }

  /**
   * \brief View as a certain type, unsafe unless union is used across basic
   * types
   */
  template <typename U>
  U& viewAs() {
    staticAssertCorrectness<U>();
    return *getUnsafe<U>();
  }

  /**
   * \brief Serialize as the right underlying type
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | this->which_;
    if (this->which_ != 0) {
      detail::Serialize<T, Ts...>::apply(this->which_, this, s);
    }
  }

  /**
   * \brief Apply a static template switch over the types in the union
   * dynamically dispatched based on the current active type
   *
   * The \c Functor passed to the function will be invoked with the \c apply
   * method templated on the \c T that is currently selected in the union. If no
   * type is currently selected, the \c apply method will be applied with
   * \c void
   *
   * Example:
   * \code{.cpp}
   *   struct MyFunctor {
   *     template <typename T>
   *     void apply(vt::adt::SafeUnion<int, float>& in);
   *   };
   *
   *   template <>
   *   void MyFunctor::apply<int>(vt::adt::SafeUnion<int, float>& in) {
   *     // triggered when `in` is storing an int
   *   }
   *
   *   template <>
   *   void MyFunctor::apply<float>(vt::adt::SafeUnion<int, float>& in) {
   *     // triggered when `in` is storing an float
   *   }
   *
   *   template <>
   *   void MyFunctor::apply<void>(vt::adt::SafeUnion<int, float>& in) {
   *     // triggered when `in` does not have a type selected
   *   }
   *
   *   void func() {
   *     vt::adt::SafeUnion<int, float> x;
   *     x.init<int>();
   *
   *     MyFunctor my_functor;
   *     x.switchOn(my_functor); // dispatches to `int` overload
   *   }
   * \endcode
   *
   * \param[in] args args to forward to functor
   */
  template <typename Functor>
  auto switchOn(Functor& fn) {
    return detail::Switch<T, Ts...>::template apply(this->which_, this, fn);
  }

public:
  template <typename U, typename SerializerT>
  void serializeAs(SerializerT& s) {
    U* t = getUnsafe<U>();
    s | *t;
  }

  template <typename U>
  bool compareAs(AlignedCharUnion const* other) const {
    U const* t1 = getUnsafe<U>();
    U const* t2 = other->getUnsafe<U>();
    return *t1 == *t2;
  }

  template <typename U>
  std::size_t hashAs() const {
    U const* t1 = getUnsafe<U>();
    return std::hash<U>()(*t1);
  }

private:
  template <typename U>
  U* getSafe() {
    staticAssertCorrectness<U>();
    vtAssert(
      (detail::Which<U, T, Ts...>::value == this->which_),
      "Must be initialized as U"
    );
    return getUnsafe<U>();
  }

  template <typename U>
  U const* getSafe() const {
    staticAssertCorrectness<U>();
    vtAssert(
      (detail::Which<U, T, Ts...>::value == this->which_),
      "Must be initialized as U"
    );
    return getUnsafe<U>();
  }

  template <typename U>
  U* getUnsafe() {
    staticAssertCorrectness<U>();
    return reinterpret_cast<U*>(static_cast<char*>(this->data_));
  }

  template <typename U>
  U const* getUnsafe() const {
    staticAssertCorrectness<U>();
    return reinterpret_cast<U const*>(static_cast<char const*>(this->data_));
  }

  template <typename U>
  void staticAssertCorrectness() const {
    detail::AllUnique<T, Ts...>{};
    static_assert(
      detail::MustBe<U, T, Ts...>::is_same, "Must be the valid type in union"
    );
  }
};

}}} /* end namespace vt::util::adt */

namespace std {

template <typename T, typename... Ts>
struct hash<vt::util::adt::AlignedCharUnion<T, Ts...>> {
  size_t operator()(
    vt::util::adt::AlignedCharUnion<T, Ts...> const& in
  ) const {
    return in.hash();
  }
};

} /* end namespace std */

namespace vt { namespace adt {

template <typename T, typename... Ts>
using SafeUnion = util::adt::AlignedCharUnion<T, Ts...>;

}} /* end namespace vt::adt */

#endif /*INCLUDED_VT_UTILS_ADT_UNION_H*/
