
#if !defined INCLUDED_EPOCH_EPOCH_MANIP_SET_H
#define INCLUDED_EPOCH_EPOCH_MANIP_SET_H

#include "config.h"
#include "epoch/epoch.h"
#include "epoch/epoch_manip.h"
#include "utils/bits/bits_common.h"
#include "utils/bits/bits_packer.h"

namespace vt { namespace epoch {

/*static*/ inline
void EpochManip::setIsRooted(EpochType& epoch, bool const is_rooted) {
  BitPackerType::boolSetField<eEpochLayout::EpochIsRooted,1,EpochType>(epoch,is_rooted);
}

/*static*/ inline
void EpochManip::setHasCategory(EpochType& epoch, bool const has_cat) {
  BitPackerType::boolSetField<eEpochLayout::EpochHasCategory,1,EpochType>(
    epoch,has_cat
  );
}

/*static*/ inline
void EpochManip::setIsUser(EpochType& epoch, bool const is_user) {
  BitPackerType::boolSetField<eEpochLayout::EpochUser,1,EpochType>(
    epoch,is_user
  );
}

/*static*/ inline
void EpochManip::setCategory(EpochType& epoch, eEpochCategory const cat) {
  BitPackerType::setField<
    eEpochLayout::EpochCategory, epoch_category_num_bits
  >(epoch,cat);
}

/*static*/ inline
void EpochManip::setNode(EpochType& epoch, NodeType const node) {
  BitPackerType::setField<eEpochLayout::EpochNode, node_num_bits>(epoch,node);
}

/*static*/ inline
void EpochManip::setSeq(EpochType& epoch, EpochType const seq) {
  BitPackerType::setField<
    eEpochLayout::EpochSequential, epoch_seq_num_bits
  >(epoch,seq);
}

}} /* end namespace vt::epoch */

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_SET_H*/
