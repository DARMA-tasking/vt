
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE_FWD__
#define __RUNTIME_TRANSPORT_SEQUENCE_FWD__

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

namespace vt {

extern std::unique_ptr<seq::Sequencer> theSeq;
extern std::unique_ptr<seq::SequencerVirtual> theVrtSeq;

} // end namespace vt

#endif /* __RUNTIME_TRANSPORT_SEQUENCE_FWD__*/
