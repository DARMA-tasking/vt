
#if !defined INCLUDED_EPOCH_EPOCH_MANIP_GET_H
#define INCLUDED_EPOCH_EPOCH_MANIP_GET_H

#include "config.h"
#include "epoch/epoch.h"
#include "epoch/epoch_manip.h"
#include "utils/bits/bits_common.h"
#include "utils/bits/bits_packer.h"

namespace vt { namespace epoch {

/*static*/ inline bool EpochManip::isRooted(EpochType const& epoch) {
  constexpr BitPackerType::FieldType field = eEpochLayout::EpochIsRooted;
  constexpr BitPackerType::FieldType size = 1;
  return BitPackerType::boolGetField<field,size,EpochType>(epoch);
}

/*static*/ inline bool EpochManip::hasCategory(EpochType const& epoch) {
  return BitPackerType::boolGetField<eEpochLayout::EpochHasCategory>(epoch);
}

/*static*/ inline bool EpochManip::isUser(EpochType const& epoch) {
  return BitPackerType::boolGetField<eEpochLayout::EpochUser>(epoch);
}

/*static*/ inline eEpochCategory EpochManip::category(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochLayout::EpochCategory, epoch_category_num_bits, eEpochCategory
  >(epoch);
}

/*static*/ inline NodeType EpochManip::node(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochLayout::EpochNode, node_num_bits, NodeType
  >(epoch);
}

/*static*/ inline EpochType EpochManip::seq(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochLayout::EpochSequential, epoch_seq_num_bits, EpochType
  >(epoch);
}

}} /* end namespace vt::epoch */

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_GET_H*/
