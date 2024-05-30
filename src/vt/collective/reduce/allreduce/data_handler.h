
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

namespace vt::collective::reduce::allreduce {
#include <vector>

#ifdef VT_KOKKOS_ENABLED
#include <Kokkos_Core.hpp>
#endif

template <typename Container>
class DataHandler {
public:
  using Scalar = float;

  static size_t size(const Container& data);
  static Scalar& at(Container& data, size_t idx);
  static void set(Container& data, size_t idx, const Scalar& value);
  static Container split(Container& data, size_t start, size_t end);
};

template <typename T>
class DataHandler<std::vector<T>> {
public:
  using UnderlyingType = std::vector<T>;
  using Scalar = T;
  static size_t size(const std::vector<T>& data) { return data.size(); }
  static T at(const std::vector<T>& data, size_t idx) { return data[idx]; }
  static T& at(std::vector<T>& data, size_t idx) { return data[idx]; }
  static void set(std::vector<T>& data, size_t idx, const T& value) {
    data[idx] = value;
  }
  static std::vector<T> split(std::vector<T>& data, size_t start, size_t end) {
    return std::vector<T>{data.begin() + start, data.begin() + end};
  }
};

#ifdef VT_KOKKOS_ENABLED
template <typename T, typename... Props>
class DataHandler<Kokkos::View<T*, Props...>> {
public:
  static size_t size(const Kokkos::View<T*, Props...>& data) {
    return data.extent(0);
  }
  static T at(const Kokkos::View<T*, Props...>& data, size_t idx) {
    return data(idx);
  }
  static T& at(Kokkos::View<T*, Props...>& data, size_t idx) {
    return data(idx);
  }
  static void
  set(Kokkos::View<T*, Props...>& data, size_t idx, const T& value) {
    data(idx) = value;
  }
};
#endif // VT_KOKKOS_ENABLED

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_DATA_HANDLER_H*/
