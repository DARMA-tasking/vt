/*
//@HEADER
// ************************************************************************
//
//                          index_example.h
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

#if !defined INCLUDED_TOPOS_INDEX_EXAMPLE
#define INCLUDED_TOPOS_INDEX_EXAMPLE

#include "vt/config.h"
#include "vt/serialization/traits/byte_copy_trait.h"

#include <cstdint>
#include <functional>
#include <string>

namespace vt { namespace index {

/*
 * This index (ExampleIndex) exists for pedagogical purposes only: to
 * demonstrate an example vt::index that conforms to the correct interface. If
 * the detector is enabled, this can be checked at compile time.
 */

struct ExampleIndex {
  using IndexSizeType = size_t;
  using ApplyType = std::function<void(ExampleIndex)>;
  using IsByteCopyable = serialization::ByteCopyTrait;

  // An index must have a default constructor
  ExampleIndex() = default;

  // An index must have a copy constructor
  ExampleIndex(ExampleIndex const&) = default;

  // An index must have an operator=
  ExampleIndex& operator=(ExampleIndex const&) = default;

  // An index must have equality defined
  bool operator==(ExampleIndex const& other) const;

  // An index must inform the runtime of its packed size
  IndexSizeType packedSize() const;

  // An index must inform the runtime if it is byte copyable
  bool indexIsByteCopyable() const;

  // Generate unique bit sequence for element index
  UniqueIndexBitType uniqueBits() const;

  // Iterator for every element in a index used as a range
  void foreach(ExampleIndex const& max, ApplyType fn) const;

  // Pretty print an index as a std::string
  std::string toString() const;
};

}} // end namespace vt::index

#if backend_check_enabled(detector)
  #include "vt/topos/index/traits/traits.h"

  namespace vt { namespace index {

  static_assert(
    IndexTraits<ExampleIndex>::is_index, "ExampleIndex does not conform"
  );

  }} // end namespace vt::index
#endif

#endif  /*INCLUDED_TOPOS_INDEX_EXAMPLE*/
