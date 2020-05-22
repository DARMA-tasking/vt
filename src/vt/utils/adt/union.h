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

namespace vt { namespace util {

namespace detail {

///////////////////////////////////////////////////////////////////////////////

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
  static constexpr char const value = sizeof...(Us) + 1;
};

///////////////////////////////////////////////////////////////////////////////
/// Find the place of a given \c T inside a pack \c U, Us...

template <typename T, typename U, typename... Us>
struct Which;

template <typename T, typename U, typename Enable, typename... Us>
struct WhichImpl;

template <typename T, typename U, typename... Us>
struct WhichImpl<T, U, typename std::enable_if_t<std::is_same<T,U>::value>, Us...> {
  static constexpr char const value = sizeof...(Us) + 1;
};

template <typename T, typename U, typename... Us>
struct WhichImpl<T, U, typename std::enable_if_t<not std::is_same<T,U>::value>, Us...> {
  static constexpr char const value = Which<T, Us...>::value;
};

template <typename T, typename U, typename... Us>
struct Which : WhichImpl<T, U, void, Us...> { };


///////////////////////////////////////////////////////////////////////////////

/// Automatically deallocate based on active element
template <typename T, typename... Ts>
struct Deallocate {
  template <typename U>
  static void apply(char which, U* u) {
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
  static void apply(char which, U* u) {
    if (which == static_cast<char>(1)) {
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
  static void apply(char which, char const* from, char* to) {
    if (which == GetPlace<Ts...>::value) {
      new (to) T{*reinterpret_cast<T const*>(from)};
    } else {
      Copy<Ts...>::apply(which, from, to);
    }
  }
};

template <typename T>
struct Copy<T> {
  static void apply(char which, char const* from, char* to) {
    if (which == static_cast<char>(1)) {
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
  static void apply(char which, char* from, char* to) {
    if (which == GetPlace<Ts...>::value) {
      new (reinterpret_cast<T*>(to)) T{std::move(*reinterpret_cast<T*>(from))};
    } else {
      Move<Ts...>::apply(which, from, to);
    }
  }
};

template <typename T>
struct Move<T> {
  static void apply(char which, char* from, char* to) {
    if (which == static_cast<char>(1)) {
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
  static void apply(char which, U* u, SerializerT& s) {
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
  static void apply(char which, U* u, SerializerT& s) {
    if (which == static_cast<char>(1)) {
      u->template serializeAs<T>(s);
    } else {
      vtAssert(false, "Invalid type; can not serialize");
    }
  }
};

} /* end namespace detail */

/**
 * \struct AlignedCharUnion
 *
 * \brief An aligned type-safe union that remembers its last type and runtime
 * checks to ensure correctness.
 *
 * Example of use:
 *
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
 *
 */
template <typename T, typename... Ts>
struct AlignedCharUnion {

  AlignedCharUnion() = default;

  AlignedCharUnion(AlignedCharUnion&& other)
    : which_(std::move(other.which_))
  {
    if (which_ != 0) {
      detail::Move<T, Ts...>::apply(which_, other.data_, data_);
    }
  }

  AlignedCharUnion(AlignedCharUnion const& other)
    : which_(other.which_)
  {
    if (which_ != 0) {
      detail::Copy<T, Ts...>::apply(which_, other.data_, data_);
    }
  }

  AlignedCharUnion& operator=(AlignedCharUnion const& other) {
    reset();
    which_ = other.which_;
    if (which_ != 0) {
      detail::Copy<T, Ts...>::apply(which_, other.data_, data_);
    }
    return *this;
  }

  AlignedCharUnion& operator=(AlignedCharUnion&& other) {
    reset();
    which_ = std::move(other.which_);
    if (which_ != 0) {
      detail::Move<T, Ts...>::apply(which_, other.data_, data_);
    }
    return *this;
  }

  ~AlignedCharUnion() { reset(); }

  /**
   * \brief Initialize as \c U with arguments to constructor \c Args
   *
   * \param[in] args constructor arguments
   *
   * \return pointer to U
   */
  template <typename U, typename... Args>
  U* init(Args&&... args) {
    staticAssertCorrectness<U>();
    vtAssert(which_ == 0, "Must be uninitialized");
    which_ = detail::Which<U, T, Ts...>::value;
    auto t = getUnsafe<U>();
    return new (t) U{std::forward<Args>(args)...};
  }

  /**
   * \brief Check if union has active type \c U
   *
   * \return whether \c U is active
   */
  template <typename U>
  bool is() {
    staticAssertCorrectness<U>();
    return which_ == detail::Which<U, T, Ts...>::value;
  }

  /**
   * \brief Reset the union, calling the appropriate destructor if one variant
   * is active.
   */
  void reset() {
    if (which_ != 0) {
      detail::Deallocate<T, Ts...>::apply(which_, this);
      which_ = 0;
    }
  }

  /**
   * \brief Deallocate as a certain type \c U
   */
  template <typename U>
  void deallocateAs() {
    staticAssertCorrectness<U>();
    vtAssert(
      (which_ == detail::Which<U, T, Ts...>::value), "Deallocating as wrong type"
    );
    reinterpret_cast<U*>(getUnsafeRawBytes())->~U();
    which_ = 0;
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
   * \brief View as a certain type, unsafe unless union is used across basic
   * types
   */
  template <typename U>
  U& viewAs() {
    staticAssertCorrectness<U>();
    return *getUnsafe<U>();
  }

  /**
   * \brief Get the char* to underlying bytes
   */
  char* getUnsafeRawBytes() { return static_cast<char*>(data_); }

  /**
   * \brief Serialize as the right underlying type
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | which_;
    if (which_ != 0) {
      detail::Serialize<T, Ts...>::apply(which_, this, s);
    }
  }

private:
  template <typename U, typename SerializerT>
  void serializeAs(SerializerT& s) {
    T* t = getUnsafe<U>();
    s | *t;
  }

  template <typename U>
  U* getSafe() {
    staticAssertCorrectness<U>();
    vtAssert(
      (detail::Which<U, T, Ts...>::value == which_), "Must be initialized as U"
    );
    return getUnsafe<U>();
  }

  template <typename U>
  U* getUnsafe() {
    staticAssertCorrectness<U>();
    return reinterpret_cast<U*>(static_cast<char*>(data_));
  }

  template <typename U>
  void staticAssertCorrectness() {
    static_assert(
      detail::MustBe<U, T, Ts...>::is_same, "Must be the valid type in union"
    );
  }

private:
  alignas(detail::Aligner<T,Ts...>) char data_[sizeof(detail::Sizer<T,Ts...>)];
  char which_ = 0;
};

}} /* end namespace vt::util */

namespace vt {

template <typename T, typename... Ts>
using SafeUnion = util::AlignedCharUnion<T, Ts...>;

} /* end namespace vt */

#endif /*INCLUDED_VT_UTILS_ADT_UNION_H*/
