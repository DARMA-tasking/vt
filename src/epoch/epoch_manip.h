
#if !defined INCLUDED_EPOCH_EPOCH_MANIP_H
#define INCLUDED_EPOCH_EPOCH_MANIP_H

#include "config.h"
#include "epoch/epoch.h"
#include "utils/bits/bits_common.h"
#include "utils/bits/bits_packer.h"

namespace vt { namespace epoch {

static constexpr NodeType const default_epoch_node = uninitialized_destination;
static constexpr eEpochCategory const default_epoch_category =
  eEpochCategory::NoCategoryEpoch;

struct EpochManip {
  /*
   *  Epoch getters to check type and state of EpochType
   */
  static bool           isRooted   (EpochType const& epoch);
  static bool           hasCategory(EpochType const& epoch);
  static bool           isUser     (EpochType const& epoch);
  static eEpochCategory category   (EpochType const& epoch);
  static NodeType       node       (EpochType const& epoch);
  static EpochType      seq        (EpochType const& epoch);

  /*
   *  Epoch setters to manipulate the type and state of EpochType
   */
  static void setIsRooted   (EpochType& epoch, bool           const is_rooted);
  static void setHasCategory(EpochType& epoch, bool           const has_cat  );
  static void setIsUser     (EpochType& epoch, bool           const is_user  );
  static void setCategory   (EpochType& epoch, eEpochCategory const cat      );
  static void setNode       (EpochType& epoch, NodeType       const node     );
  static void setSeq        (EpochType& epoch, EpochType      const seq      );

  static EpochType makeEpoch(
    EpochType      const& seq,
    bool           const& is_rooted  = false,
    NodeType       const& root_node  = default_epoch_node,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );
  static EpochType next(EpochType const& epoch);

private:
  static EpochType nextSlow(EpochType const& epoch);
  static EpochType nextFast(EpochType const& epoch);
};

}} /* end namespace vt::epoch */

#include "epoch/epoch_manip_get.h"
#include "epoch/epoch_manip_set.h"
#include "epoch/epoch_manip_make.h"

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_H*/
