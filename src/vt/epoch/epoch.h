
#if !defined INCLUDED_EPOCH_EPOCH_H
#define INCLUDED_EPOCH_EPOCH_H

#include "vt/config.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace epoch {

/*
 * ========================= Layout of the Epoch =========================
 *
 *   w-1 .............. w-h-1 ...............w-h-c-1 ....................0
 *   | <EpochHeader> ... | <EpochCategory> ... | <Sequential Epoch ID>   |
 *
 *      *where*    h = epoch_header_num_bits,
 *                 c = epoch_category_num_bits,
 *                 w = sizeof(EpochType) * 8
 *                 n = sizeof(NodeType)        ^             ^           ^
 *                                             | .... n .... | ..........|
 *                                               <NodeType>  <SeqEpochID>
 *
 *  +++++++++++++++++++++++++++++++++++++++++++  Rooted Extended Layout ++
 *
 *   <EpochHeader>   = <IsRooted> <HasCategory> <IsUser>
 *   ....3 bits...   = ..bit 1..   ...bit 2...  ..bit 3..
 *
 * =======================================================================
 */

enum struct eEpochHeader : int8_t {
  RootedEpoch   = 1,
  CategoryEpoch = 2,
  UserEpoch     = 3
};

static constexpr BitCountType const epoch_root_num_bits = 1;
static constexpr BitCountType const epoch_hcat_num_bits = 1;
static constexpr BitCountType const epoch_user_num_bits = 1;

/*
 *  Important: if you add new types of epoch headers to the preceding enum, you
 *  must ensure that the number of epoch header bits is sufficient to hold all
 *  the header types.
 *
 */

enum struct eEpochCategory : int8_t {
  NoCategoryEpoch = 0x0,
  InsertEpoch     = 0x1
};

/*
 *  Important: if you add categories to the enum of epoch categories, you must
 *  ensure the epoch_category_num_bits is sufficiently large.
 *
 */
static constexpr BitCountType const epoch_category_num_bits = 1;

/*
 *  Epoch layout enum to help with manipuating the bits
 */

static constexpr BitCountType const epoch_seq_num_bits = sizeof(EpochType) * 8 -
  (epoch_root_num_bits     +
   epoch_hcat_num_bits     + epoch_user_num_bits +
   epoch_category_num_bits + node_num_bits);

enum eEpochLayout {
  EpochSequential   = 0,
  EpochNode         = eEpochLayout::EpochSequential  + epoch_seq_num_bits,
  EpochCategory     = eEpochLayout::EpochNode        + node_num_bits,
  EpochUser         = eEpochLayout::EpochCategory    + epoch_category_num_bits,
  EpochHasCategory  = eEpochLayout::EpochUser        + epoch_user_num_bits,
  EpochIsRooted     = eEpochLayout::EpochHasCategory + epoch_hcat_num_bits,
  EpochSentinelEnd  = eEpochLayout::EpochIsRooted
};

/*
 *  The first basic epoch: BasicEpoch, NoCategoryEpoch:
 */

static constexpr EpochType const first_epoch = 1;

}} //end namespace vt::epoch

#endif /*INCLUDED_EPOCH_EPOCH_H*/
