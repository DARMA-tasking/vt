/*
//@HEADER
// *****************************************************************************
//
//                                 op_buffer.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_OP_BUFFER_H
#define INCLUDED_VT_VRT_COLLECTION_OP_BUFFER_H

#include "vt/config.h"

namespace vt { namespace vrt { namespace collection {

/**
 *  \brief The type of operation being buffered
 */
enum BufferTypeEnum {
  Broadcast = 0x1,
  Send      = 0x2,
  Reduce    = 0x4
};

/**
 *  \brief List of potential buffer release triggers that can be composed |
 */
enum BufferReleaseEnum {
  Unreleased            = 0x0, /**< Sentinel value */
  AfterFullyConstructed = 0x1, /**< After the collection construct reduction */
  AfterMetaDataKnown    = 0x2, /**< After meta data is known */
  AfterGroupReady       = 0x4  /**< After the underlying group is created */
};

}}} /* end namespace vt::vrt::collection */

namespace std {

using BufferTypeEnum = vt::vrt::collection::BufferTypeEnum;
using BufferReleaseEnum = vt::vrt::collection::BufferReleaseEnum;

template <>
struct hash<BufferTypeEnum> {
  size_t operator()(BufferTypeEnum in) const {
    using UnderType = typename std::underlying_type<BufferTypeEnum>::type;
    auto const val = static_cast<UnderType>(in);
    return std::hash<UnderType>()(val);
  }
};

template <>
struct hash<BufferReleaseEnum> {
  size_t operator()(BufferReleaseEnum in) const {
    using UnderType = typename std::underlying_type<BufferReleaseEnum>::type;
    auto const val = static_cast<UnderType>(in);
    return std::hash<UnderType>()(val);
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_VRT_COLLECTION_OP_BUFFER_H*/
