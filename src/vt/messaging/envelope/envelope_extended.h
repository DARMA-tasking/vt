/*
//@HEADER
// *****************************************************************************
//
//                             envelope_extended.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

#include <type_traits>

namespace vt { namespace messaging {

/** \file */

/**
 * \struct EpochActiveEnvelope
 *
 * \brief Extended envelope that holds an epoch, contains all of \c
 * ActiveEnvelope
 */
struct EpochActiveEnvelope {
  using isByteCopyable = std::true_type;

  ActiveEnvelope env;               /**< The basic envelope */
  EpochType epoch : epoch_num_bits; /**< The epoch */
};

/**
 * \struct EpochActiveEnvelope
 *
 * \brief Extended envelope that holds a tag, contains all of \c ActiveEnvelope
 */
struct TagActiveEnvelope {
  using isByteCopyable = std::true_type;

  ActiveEnvelope env;               /**< The basic envelope */
  TagType tag     : tag_num_bits;   /**< The tag */
};

/**
 * \struct EpochActiveEnvelope
 *
 * \brief Extended envelope that holds an epoch and tag, contains all of \c
 * ActiveEnvelope
 */
struct EpochTagActiveEnvelope {
  using isByteCopyable = std::true_type;

  ActiveEnvelope env;               /**< The basic envelope */
  EpochType epoch : epoch_num_bits; /**< The epoch */
  TagType tag     : tag_num_bits;   /**< The tag */
};

}} //end namespace vt::messaging

namespace vt {

using EpochEnvelope    = messaging::EpochActiveEnvelope;
using TagEnvelope      = messaging::TagActiveEnvelope;
using EpochTagEnvelope = messaging::EpochTagActiveEnvelope;

static_assert(std::is_pod<EpochEnvelope>(),    "EpochEnvelope must be POD");
static_assert(std::is_pod<TagEnvelope>(),      "TagEnvelope must be POD");
static_assert(std::is_pod<EpochTagEnvelope>(), "EpochTagEnvelope must be POD");
static_assert(
  std::is_trivially_destructible<EpochEnvelope>(),
  "EpochEnvelope must be trivial"
);
static_assert(
  std::is_trivially_destructible<TagEnvelope>(),
  "TagEnvelope must be trivial"
);
static_assert(
  std::is_trivially_destructible<EpochTagEnvelope>(),
  "EpochTagEnvelope must be trivial"
);

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_extended.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_H*/
