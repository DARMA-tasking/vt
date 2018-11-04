
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

#include <type_traits>

namespace vt { namespace messaging {

struct EpochActiveEnvelope {
  using isByteCopyable = std::true_type;

  ActiveEnvelope env;
  EpochType epoch : epoch_num_bits;
};

struct TagActiveEnvelope {
  using isByteCopyable = std::true_type;

  ActiveEnvelope env;
  TagType tag     : tag_num_bits;
};

struct EpochTagActiveEnvelope {
  using isByteCopyable = std::true_type;

  ActiveEnvelope env;
  EpochType epoch : epoch_num_bits;
  TagType tag     : tag_num_bits;
};

}} //end namespace vt::messaging

namespace vt {

using EpochEnvelope    = messaging::EpochActiveEnvelope;
using TagEnvelope      = messaging::TagActiveEnvelope;
using EpochTagEnvelope = messaging::EpochTagActiveEnvelope;

static_assert(std::is_pod<EpochEnvelope>(),    "EpochEnvelope must be POD");
static_assert(std::is_pod<TagEnvelope>(),      "TagEnvelope must be POD");
static_assert(std::is_pod<EpochTagEnvelope>(), "EpochTagEnvelope must be POD");

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_extended.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_H*/
