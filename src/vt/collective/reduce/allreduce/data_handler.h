/*
//@HEADER
// *****************************************************************************
//
//                                data_handler.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_DATA_HANDLER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_DATA_HANDLER_H

#include "vt/configs/error/config_assert.h"

#include <vector>

#ifdef MAGISTRATE_KOKKOS_ENABLED
#include <Kokkos_Core.hpp>
#endif // MAGISTRATE_KOKKOS_ENABLED

namespace vt::collective::reduce::allreduce {

template <typename DataType, typename Enable = void>
class DataHandler {
public:
  using Scalar = void;

  static DataType toVec(const DataType&) {
    vtAssert(
      true,
      "Using default DataHandler! This means that you're using custom type for "
      "allreduce. Please provide specialization for you data type."
    );

    return {};
  }
  static DataType fromVec(const std::vector<Scalar>&) {
    vtAssert(
      true,
      "Using default DataHandler! This means that you're using custom type for "
      "allreduce. Please provide specialization for you data type."
    );

    return {};
  }

  static DataType fromMemory(const Scalar*, size_t) {
    vtAssert(
      true,
      "Using default DataHandler! This means that you're using custom type for "
      "allreduce. Please provide specialization for you data type."
    );

    return {};
  }

  static size_t size(void) {
    vtAssert(
      true,
      "Using default DataHandler! This means that you're using custom type for "
      "allreduce. Please provide specialization for you data type."
    );

    return {};
  }
};

template <typename ScalarType>
class DataHandler<ScalarType, typename std::enable_if<std::is_arithmetic<ScalarType>::value>::type> {
public:
  using Scalar = ScalarType;

  static std::vector<ScalarType> toVec(const ScalarType& data) { return std::vector<ScalarType>{data}; }
  static ScalarType fromVec(const std::vector<ScalarType>& data) { return data[0]; }
  static ScalarType fromMemory(const ScalarType* data, size_t) {
    return *data;
  }

  static size_t size(const ScalarType&) { return 1; }
};

template <typename T>
class DataHandler<std::vector<T>> {
public:
  using Scalar = T;

  static const std::vector<T>& toVec(const std::vector<T>& data) { return data; }
  static std::vector<T> fromVec(const std::vector<T>& data) { return data; }
  static std::vector<T> fromMemory(const T* data, size_t count) {
    return std::vector<T>(data, data + count);
  }

  static size_t size(const std::vector<T>& data) { return data.size(); }
};

#if MAGISTRATE_KOKKOS_ENABLED

template <typename T, typename... Props>
class DataHandler<Kokkos::View<T*, Kokkos::HostSpace, Props...>> {
  using ViewType = Kokkos::View<T*, Kokkos::HostSpace, Props...>;

public:
  using Scalar = T;

  static std::vector<T> toVec(const ViewType& data) {
    std::vector<T> vec;
    vec.resize(data.extent(0));
    std::memcpy(vec.data(), data.data(), data.extent(0) * sizeof(T));
    return vec;
  }

  static ViewType fromMemory(T* data, size_t size) {
    return ViewType(data, size);
  }

  static ViewType fromVec(const std::vector<T>& data) {
    ViewType view("", data.size());
    Kokkos::parallel_for(
      "InitView", view.extent(0),
      KOKKOS_LAMBDA(const int i) { view(i) = static_cast<float>(data[i]); });

    return view;
  }

  static size_t size(const ViewType& data) { return data.extent(0); }
};

#endif // MAGISTRATE_KOKKOS_ENABLED

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_DATA_HANDLER_H*/
