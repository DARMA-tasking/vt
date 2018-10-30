
#if !defined INCLUDED_SEQUENCE_SEQUENCER_FWD_H
#define INCLUDED_SEQUENCE_SEQUENCER_FWD_H

#include "config.h"
#include "seq_common.h"

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencer;

using Sequencer = TaggedSequencer<SeqType, SeqMigratableTriggerType>;

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencerVrt;

using SequencerVirtual = TaggedSequencerVrt<SeqType, SeqMigratableTriggerType>;

}} // end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQUENCER_FWD_H*/
