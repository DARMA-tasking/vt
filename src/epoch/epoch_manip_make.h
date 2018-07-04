
#if !defined INCLUDED_EPOCH_EPOCH_MANIP_MAKE_H
#define INCLUDED_EPOCH_EPOCH_MANIP_MAKE_H

#include "config.h"
#include "epoch/epoch.h"
#include "epoch/epoch_manip.h"
#include "utils/bits/bits_common.h"
#include "utils/bits/bits_packer.h"

namespace vt { namespace epoch {

/*static*/ inline EpochType EpochManip::makeEpoch(
  EpochType const& seq, bool const& is_rooted, NodeType const& root_node,
  bool const& is_user, eEpochCategory const& category
) {
  EpochType new_epoch = 0;
  bool const& has_category = category != eEpochCategory::NoCategoryEpoch;
  EpochManip::setIsRooted(new_epoch, is_rooted);
  EpochManip::setIsUser(new_epoch, is_user);
  EpochManip::setHasCategory(new_epoch, has_category);
  if (is_rooted) {
    assert(root_node != uninitialized_destination);
    EpochManip::setNode(new_epoch, root_node);
  }
  if (has_category) {
    EpochManip::setCategory(new_epoch, category);
  }
  EpochManip::setSeq(new_epoch, seq);
  return new_epoch;
}

/*static*/ inline EpochType EpochManip::next(EpochType const& epoch) {
  return EpochManip::nextFast(epoch);
}

/*static*/ inline EpochType EpochManip::nextSlow(EpochType const& epoch) {
  EpochType new_epoch = epoch;
  auto const& this_seq = EpochManip::seq(epoch);
  EpochManip::setSeq(new_epoch, this_seq + 1);
  return new_epoch;
}

/*static*/ inline EpochType EpochManip::nextFast(EpochType const& epoch) {
  return epoch + 1;
}

// /*static*/ inline EpochType EpochManip::makeCollEpoch(
//   EpochType seq, eEpochHeader hdr, eEpochCategory category
// ) {
//   return EpochType{};
// }

// /*static*/ inline EpochType EpochManip::getSeq(EpochType const& epoch) {
//   // auto const& is_rooted = isRooted(epoch);

//   // if (!is_rooted) {
//   //   return BitPackerType::getField<
//   //     eEpochLayout::Sequential, epoch_seq_num_bits, EpochType
//   //   >(epoch);
//   // } else {
//   //   return BitPackerType::getField<
//   //     eEpochRootedLayout::RootedSeq, epoch_rooted_seq_num_bits, EpochType
//   //   >(epoch);
//   // }
// }

}} /* end namespace vt::epoch */

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_MAKE_H*/
