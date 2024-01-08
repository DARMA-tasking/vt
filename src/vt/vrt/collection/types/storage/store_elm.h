/*
//@HEADER
// *****************************************************************************
//
//                                 store_elm.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORE_ELM_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORE_ELM_H

#include "vt/config.h"

#include <nlohmann/json.hpp>

#include <checkpoint/checkpoint.h>

#include <type_traits>
#include <variant>

namespace vt { namespace vrt { namespace collection { namespace storage {

/**
 * \struct StoreElmBase
 *
 * \brief Polymorphic, untyped base class for stored values in collection
 * elements.
 */
struct StoreElmBase {
  /// uses polymorphic serialization
  checkpoint_virtual_serialize_root()

  using json = nlohmann::json;

  StoreElmBase() = default;

  /**
   * \brief Store element base class constructor
   *
   * \param[in] dump_to_json whether to dump to JSON output
   * \param[in] provide_to_lb whether to provide to the LB
   */
  StoreElmBase(bool dump_to_json, bool provide_to_lb)
    : dump_to_json_(dump_to_json),
      provide_to_lb_(provide_to_lb)
  {}

  virtual ~StoreElmBase() {}

  /**
   * \brief Generate the json if applicable
   *
   * \return the json
   */
  virtual nlohmann::json toJson() = 0;

  /**
   * \brief Generate variant for LB
   *
   * \return the variant
   */
  virtual std::variant<int, double, std::string> toVariant() = 0;

  /**
   * \brief Get the value as \c T
   *
   * \return the value
   */
  template <typename T>
  T& get();

  /**
   * \brief Get the const value as \c T
   *
   * \return the value
   */
  template <typename T>
  T const& get() const;

  /**
   * \brief Serialize the base class
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | dump_to_json_;
    s | provide_to_lb_;
  }

  /**
   * \brief Whether the value should be dumped to the json LB data file
   *
   * \return whether to dump
   */
  bool shouldJson() const { return dump_to_json_; }

  /**
   * \brief Whether to provide to the LB
   *
   * \return is it for LB
   */
  bool provideToLB() const {
    return provide_to_lb_;
  }

  /**
   * \brief Generate the json because it is jsonable
   *
   * \param[in] u the data to convert to json
   */
  template <typename U>
  static json maybeGenerateJson(U const& u) {
    if constexpr (nlohmann::detail::has_to_json<json,U>::value) {
      json j(u);
      return j;
    } else {
      vtAbort("Instantiated maybeGenerateJson on non-jsonable type");
      return json{};
    }
  }

  /**
   * \brief Generate the variant if matches types
   *
   * \param[in] u the data to convert to json
   */
  template <typename U>
  static auto maybeGenerateVariant(U const& u) {
    if constexpr (
      std::is_same_v<U, int> or
      std::is_same_v<U, double> or
      std::is_same_v<U, std::string>
    ) {
      return std::variant<int, double, std::string>{u};
    } else {
      vtAbort("Instantiated maybeGenerateVariant on type that doesn't apply");
      return std::variant<int, double, std::string>{};
    }
  }

protected:
  bool dump_to_json_ = false;
  bool provide_to_lb_ = false;
};

/**
 * \struct StoreElm
 *
 * \brief Typed value stored in collection element
 */
template <typename T, typename Enable = void>
struct StoreElm;

/**
 * \struct StoreElm
 *
 * \brief Typed value specialized for serializable stored values
 */
template <typename T>
struct StoreElm<
  T,
  typename std::enable_if_t<
    ::checkpoint::SerializableTraits<T>::is_serializable
  >
> : StoreElmBase
{
  /// polymorphic serializer for derived class
  checkpoint_virtual_serialize_derived_from(StoreElmBase)

  /**
   * \brief Construct with value
   *
   * \param[in] u the value
   * \param[in] dump_to_json whether to dump to json
   * \param[in] provide_to_lb whether to provide to LB
   */
  template <typename U>
  explicit StoreElm(U&& u, bool dump_to_json, bool provide_to_lb)
    : StoreElmBase(dump_to_json, provide_to_lb),
      elm_(std::forward<U>(u))
  { }

  /**
   * \brief Serialization re-constructor
   *
   * \param[in] SERIALIZE_CONSTRUCT_TAG tag
   */
  explicit StoreElm(checkpoint::SERIALIZE_CONSTRUCT_TAG) {}

  /**
   * \Brief Serializer
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    StoreElmBase::serialize(s);
    s | elm_;
  }

  /**
   * \brief Get the underlying value
   *
   * \return the value
   */
  T& get() { return elm_; }

  /**
   * \brief Get the underlying value as const
   *
   * \return the value
   */
  T const& get() const { return elm_; }

  /**
   * \brief Generate the json if jsonable
   *
   * \return the json
   */
  nlohmann::json toJson() override {
    return StoreElm::maybeGenerateJson<T>(elm_);
  }

  /**
   * \brief Generate variant if applicable
   *
   * \return the variant
   */
  std::variant<int, double, std::string> toVariant() override {
    return StoreElm::maybeGenerateVariant<T>(elm_);
  }

private:
  T elm_ = {};                  /**< The stored value */
};

namespace detail {

/**
 * \internal \struct ByteWrapper
 *
 * \brief Wrapper for byte-copyable value with type trait for serializer
 * framework.
 */
template <typename T>
struct ByteWrapper {
  /// Trait for serializer
  using isByteCopyable = std::true_type;

  /// Constructor for serialization
  ByteWrapper() = default;

  /**
   * \brief Construct wrapper with value
   *
   * \param[in] u the value
   */
  template <typename U>
  explicit ByteWrapper(U&& u)
    : elm_(std::forward<U>(u))
  { }

  T elm_ = {};                  /**< The stored, byte-copyable value */
};

} /* end detail namespace */

/**
 * \struct StoreElm
 *
 * \brief Typed value specialized for byte-copyable stored values
 */
template <typename T>
struct StoreElm<
  T,
  typename std::enable_if_t<
    not ::checkpoint::SerializableTraits<T>::is_serializable
  >
> : StoreElmBase
{
  /// polymorphic serializer for derived class
  checkpoint_virtual_serialize_derived_from(StoreElmBase)

  static_assert(
    std::is_trivially_copyable<T>::value and not std::is_pointer<T>::value,
    "Non-serializable must always at least be trivially copyable and not a pointer"
  );

  /**
   * \brief Construct with value
   *
   * \param[in] u the value
   * \param[in] dump_to_json whether to dump to json
   * \param[in] provide_to_lb whether to provide to LB
   */
  template <typename U>
  explicit StoreElm(U&& u, bool dump_to_json, bool provide_to_lb)
    : StoreElmBase(dump_to_json, provide_to_lb),
      wrapper_(detail::ByteWrapper<T>{std::forward<U>(u)})
  { }

  /**
   * \brief Serialization re-constructor
   *
   * \param[in] SERIALIZE_CONSTRUCT_TAG tag
   */
  explicit StoreElm(checkpoint::SERIALIZE_CONSTRUCT_TAG) {}

  /**
   * \brief Serializer
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    StoreElmBase::serialize(s);
    s | wrapper_;
  }

  /**
   * \brief Get the underlying value
   *
   * \return the value
   */
  T& get() { return wrapper_.elm_; }

  /**
   * \brief Get the underlying value as const
   *
   * \return the value
   */
  T const& get() const { return wrapper_.elm_; }

  /**
   * \brief Generate the json if jsonable
   *
   * \return the json
   */
  nlohmann::json toJson() override {
    return StoreElm::maybeGenerateJson<T>(wrapper_.elm_);
  }

  /**
   * \brief Generate variant if applicable
   *
   * \return the variant
   */
  std::variant<int, double, std::string> toVariant() override {
    return StoreElm::maybeGenerateVariant<T>(wrapper_.elm_);
  }

private:
  detail::ByteWrapper<T> wrapper_ = {}; /**< The byte-copyable value wrapper */
};

}}}} /* end namespace vt::vrt::collection::storage */

#include "vt/vrt/collection/types/storage/store_elm.impl.h"

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORE_ELM_H*/
