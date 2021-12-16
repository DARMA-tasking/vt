/*
//@HEADER
// *****************************************************************************
//
//                                   elm_id.h
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

#if !defined INCLUDED_VT_ELM_ELM_ID_H
#define INCLUDED_VT_ELM_ELM_ID_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"

namespace vt { namespace elm {

/// The underlying element ID type
using ElementIDType = uint64_t;

/// Sentinel value for no element ID
static constexpr ElementIDType const no_element_id = 0;

/**
 * \struct ElementIDStruct
 *
 * \brief A general identifier for a task context. The \c id is unique in the
 * system.
 */
struct ElementIDStruct {
  using isByteCopyable = std::true_type;

  bool operator==(const ElementIDStruct& rhs) const {
    return id == rhs.id;
  }

  bool operator<(const ElementIDStruct& rhs) const {
    return id < rhs.id;
  }

  ElementIDType id = no_element_id; /**< id must be unique across nodes */
  NodeType curr_node = uninitialized_destination; /**< the current node */

  bool isMigratable() const;
  NodeType getHomeNode() const;
  NodeType getCurrNode() const;
};


}} /* end namespace vt::elm */

namespace std {

template <>
struct hash<vt::elm::ElementIDStruct> {
  size_t operator()(vt::elm::ElementIDStruct const& in) const {
    return std::hash<vt::elm::ElementIDType>()(in.id);
  }
};

} /* end namespace std */

#include <fmt/format.h>

namespace fmt {

/// Custom fmt formatter/print for \c vt::elm::ElementIDStruct
template <>
struct formatter<vt::elm::ElementIDStruct> {
  /// Presentation format:
  ///  - 'x' - hex (default)
  ///  - 'd' - decimal
  ///  - 'b' - binary
  char presentation = 'x';

  /// Parses format specifications of the form ['x' | 'd' | 'b'].
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'x' || *it == 'd' || *it == 'b')) {
      presentation = *it++;
    }

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  /// Formats the epoch using the parsed format specification (presentation)
  /// stored in this formatter.
  template <typename FormatContext>
  auto format(vt::elm::ElementIDStruct const& e, FormatContext& ctx) {
    std::string id_format =
      presentation == 'b' ? "{:b}" : (presentation == 'd' ? "{:d}" : "{:x}");
    auto fmt_str = "(" + id_format + ",{},{},{})";
    return format_to(
      ctx.out(), fmt_str, e.id, e.getHomeNode(), e.curr_node, e.isMigratable()
    );
  }
};

} /* end namespace fmt */

#endif /*INCLUDED_VT_ELM_ELM_ID_H*/
