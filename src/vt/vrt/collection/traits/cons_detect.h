/*
//@HEADER
// ************************************************************************
//
//                          cons_detect.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_VRT_COLLECTION_CONS_DETECT_H
#define INCLUDED_VRT_COLLECTION_CONS_DETECT_H

#include "vt/config.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /* backend_check_enabled(detector) */

#include <functional>

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename... Args>
struct ConstructorType {
  template <typename U>
  using non_idx_t = decltype(U(std::declval<Args>()...));
  template <typename U>
  using idx_fst_t = decltype(U(std::declval<IndexT>(),std::declval<Args>()...));
  template <typename U>
  using idx_snd_t = decltype(U(std::declval<Args>()...,std::declval<IndexT>()));

  using has_non_index_cons = detection::is_detected<non_idx_t, ColT>;
  using has_index_fst      = detection::is_detected<idx_fst_t, ColT>;
  using has_index_snd      = detection::is_detected<idx_snd_t, ColT>;

  static constexpr auto const non_index_cons = has_non_index_cons::value;
  static constexpr auto const index_fst      = has_index_fst::value;
  static constexpr auto const index_snd      = has_index_snd::value;

  static constexpr auto const use_no_index   =
    non_index_cons && !index_snd && !index_fst;
  static constexpr auto const use_index_fst  =
    index_fst;
  static constexpr auto const use_index_snd  =
    index_snd && !index_fst;
};

}}} /* end namespace vt::vrt::collection */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_CONS_DETECT_H*/
