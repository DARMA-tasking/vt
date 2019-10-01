/*
//@HEADER
// *****************************************************************************
//
//                                mutex_traits.h
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

#if !defined INCLUDED_UTILS_MUTEX_MUTEX_TRAITS_H
#define INCLUDED_UTILS_MUTEX_MUTEX_TRAITS_H

#include "vt/config.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /*backend_check_enabled(detector)*/

#if backend_check_enabled(detector)

namespace vt { namespace util { namespace mutex {

template <typename T>
struct MutexTraits {
  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using has_constructor = detection::is_detected<constructor_t, T>;

  template <typename U>
  using copy_constructor_t = decltype(U(std::declval<U const&>()));
  using has_copy_constructor = detection::is_detected<copy_constructor_t, T>;

  template <typename U>
  using lock_t = decltype(std::declval<U>().lock());
  using has_lock = detection::is_detected<lock_t, T>;

  template <typename U>
  using unlock_t = decltype(std::declval<U>().unlock());
  using has_unlock = detection::is_detected<unlock_t, T>;

  template <typename U>
  using try_lock_t = decltype(std::declval<U>().try_lock());
  using has_try_lock = detection::is_detected_exact<bool, try_lock_t, T>;

  // This defines what it means to be an `mutex'
  static constexpr auto const is_mutex =
    // default constructor and copy constructor
    has_constructor::value and not has_copy_constructor::value and
    // methods: lock(), unlock(), try_lock()
    has_lock::value and has_unlock::value and
    has_try_lock::value;
};

}}} /* end namespace vt::util::mutex */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_UTILS_MUTEX_MUTEX_TRAITS_H*/
