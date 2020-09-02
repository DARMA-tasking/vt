/*
//@HEADER
// *****************************************************************************
//
//                              maybe_serialize.h
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

#if !defined INCLUDED_VT_UTILS_ADT_MAYBE_SERIALIZE_H
#define INCLUDED_VT_UTILS_ADT_MAYBE_SERIALIZE_H

#include "vt/utils/adt/union.h"

#include <memory>

namespace vt { namespace util { namespace adt {

namespace detail {

////////////////////////////////////////////////////////////////////

/**
 * \internal \struct Bytes
 *
 * \brief Contains a fixed-size payload with max alignment so any \c T can be
 * safely put inside if \c sizeof(T)<k
 */
template <std::size_t k>
struct Bytes {
  alignas(max_align_t) char data[k];
};

////////////////////////////////////////////////////////////////////

/**
 * \internal \struct SharedUnionBase
 *
 * \brief Either holds a managed pointer to \c T or safes \c T inline in the
 * sate union. Like a short-string optimization for serialization.
 */
template <std::size_t k>
struct SharedUnionBase {

  SharedUnionBase() = default;

protected:
  bool isBytes() const {
    return payload.template is<Bytes<k>>();
  }

  template <typename T>
  void setBytes(T* ptr) {
    payload.reset();
    payload.template init<Bytes<k>>();
    auto& bytes = payload.template get<Bytes<k>>();
    auto bytes_ptr = static_cast<char*>(bytes.data);
    T* bytes_t = reinterpret_cast<T*>(bytes_ptr);
    *bytes_t = *ptr;
  }

  template <typename T>
  T* getBytes() {
    vtAssert(payload.template is<Bytes<k>>(), "Must be of bytes type");
    auto bytes = payload.template get<Bytes<k>>();
    auto bytes_ptr = static_cast<char*>(bytes.data);
    return reinterpret_cast<T*>(bytes_ptr);
  }

  template <typename T>
  T* const getBytes() const {
    vtAssert(payload.template is<Bytes<k>>(), "Must be of bytes type");
    auto bytes = payload.template get<Bytes<k>>();
    auto bytes_ptr = static_cast<char*>(bytes.data);
    return reinterpret_cast<T* const>(bytes_ptr);
  }

  bool isPtr() const {
    return payload.template is<std::shared_ptr<void>>();
  }

  template <typename T>
  void setPtr(T* ptr) {
    auto ptr_copy = std::make_shared<T>(*ptr);
    payload.reset();
    payload.template init<std::shared_ptr<void>>(
      std::static_pointer_cast<void, T>(ptr_copy)
    );
  }

  template <typename T>
  T* getPtr() {
    vtAssert(payload.template is<std::shared_ptr<void>>(), "Must be shared ptr");
    auto ptr = payload.template get<std::shared_ptr<void>>();
    return static_cast<T*>(ptr.get());
  }

  template <typename T>
  T* const getPtr() const {
    vtAssert(payload.template is<std::shared_ptr<void>>(), "Must be shared ptr");
    auto& ptr = payload.template get<std::shared_ptr<void>>();
    return static_cast<T* const>(ptr.get());
  }

private:
  vt::adt::SafeUnion<Bytes<k>, std::shared_ptr<void>> payload;
};

/////////////////////////////////////////////////////////////////////

} /* end namespace detail */

/**
 * \struct MaybeSerialize
 *
 * \brief A wrapper type that contains \c k bytes to store \c T inline or a
 * pointer to \c T if not. If \c sizeof(T)<k \c T will be stored directly in the
 * struct and it can be byte serialized, like a short string
 * optimization. Otherwise, \c T will be stored in a shared pointer and the
 * serialize method will be included.
 */
template <typename T, std::size_t k, typename=void>
struct MaybeSerialize;

template <typename T, std::size_t k>
struct MaybeSerialize<
  T, k, typename std::enable_if_t<sizeof(T) >= k>
> : detail::SharedUnionBase<k>
{
  using BaseType = detail::SharedUnionBase<k>;

  /// Whether the struct will be serialized or not
  static constexpr bool const will_serialize = true;

  /**
   * \brief Get a copy of the stored \c T
   *
   * \return the stored \c T
   */
  T get() const {
    auto p = BaseType::template getPtr<T>();
    return *p;
  }

  /**
   * \brief Set \c T that is stored
   *
   * \param[in] t the \c t to store
   */
  template <typename U>
  void set(U&& t) {
    BaseType::template setPtr<T>(&t);
  }

  /**
   * \brief Serialize the pointer to \c T since \c sizeof(T) is larger than \c k
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    if (s.isUnpacking()) {
      T packed = {};
      s | packed;
      BaseType::template setPtr<T>(&packed);
    } else {
      auto p = BaseType::template getPtr<T>();
      s | *p;
    }
  }
};

template <typename T, std::size_t k>
struct MaybeSerialize<
  T, k, typename std::enable_if_t<not(sizeof(T) >= k)>
> : detail::SharedUnionBase<k>
{
  using BaseType = detail::SharedUnionBase<k>;

  /// Whether the struct will be serialized or not
  static constexpr bool const will_serialize = false;

  static_assert(
    std::is_trivially_destructible<T>::value,
    "Type must be trivially destructible"
  );

  /**
   * \brief Get a copy of the stored \c T
   *
   * \return the stored \c T
   */
  T get() const {
    auto p = BaseType::template getBytes<T>();
    return *p;
  }

  /**
   * \brief Set \c T that is stored
   *
   * \param[in] t the \c t to store
   */
  template <typename U>
  void set(U&& t) {
    BaseType::template setBytes<T>(&t);
  }

};

}}} /* end namespace vt::util::adt */

namespace vt { namespace adt {

template <typename T, std::size_t k>
using MaybeSerialize = vt::util::adt::MaybeSerialize<T, k>;

}} /* end namespace vt::adt */

#endif /*INCLUDED_VT_UTILS_ADT_MAYBE_SERIALIZE_H*/
