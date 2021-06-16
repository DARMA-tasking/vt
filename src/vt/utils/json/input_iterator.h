/*
//@HEADER
// *****************************************************************************
//
//                               input_iterator.h
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

#if !defined INCLUDED_VT_UTILS_JSON_INPUT_ITERATOR_H
#define INCLUDED_VT_UTILS_JSON_INPUT_ITERATOR_H

#include "vt/utils/json/decompression_input_container.h"

namespace vt { namespace util { namespace json {

/**
 * \struct InputIterator
 *
 * \brief Input iterator for compressed input data.
 */
struct InputIterator {
  // Not going to support other UTF encodings for now.
  using CharType = char;

  using difference_type = std::ptrdiff_t;
  using value_type = CharType;
  using pointer = CharType const*;
  using reference = CharType const&;
  using iterator_category = std::input_iterator_tag;

  InputIterator() = default;
  explicit InputIterator(DecompressionInputContainer const* in_c)
    : c_(in_c)
  { }

  InputIterator& operator++() {
    if (not c_->advance()) {
      // we are at the end, set to nullptr
      c_ = nullptr;
    }
    return *this;
  }

  bool operator!=(InputIterator const& rhs) const {
    return rhs.c_ != c_;
  }

  reference operator*() const {
    return c_->getCurrent();
  }

private:
  /// The underlying container with the data.
  /// @note: We lie about constness here to satisfy the iterator concept: the
  /// \c DecompressionInputContainer can not traverse the data in a stateless
  /// manner because it decompresses with chunks.
  DecompressionInputContainer const* c_ = nullptr;
};

/**
 * \brief ADL function for creating the begin iterator
 *
 * \param[in] c the container
 *
 * \return the iterator
 */
inline InputIterator begin(DecompressionInputContainer const& c) {
  return InputIterator{&c};
}

/**
 * \brief ADL function for creating the end iterator
 *
 * \return the iterator
 */
inline InputIterator end(DecompressionInputContainer const&) {
  return InputIterator{};
}

}}} /* end namespace vt::util::json */

#endif /*INCLUDED_VT_UTILS_JSON_INPUT_ITERATOR_H*/
