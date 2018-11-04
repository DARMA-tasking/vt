
#if !defined INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_H
#define INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_H

#include "vt/config.h"
#include "vt/sequence/sequencer.h"
#include "vt/sequence/seq_virtual_info.h"
#include "vt/sequence/seq_matcher_virtual.h"
#include "vt/sequence/seq_action_virtual.h"
#include "vt/sequence/seq_state_virtual.h"
#include "vt/vrt/context/context_vrt.h"

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencerVrt : TaggedSequencer<SeqTag, SeqTrigger> {
  using SeqType = SeqTag;
  using Base = TaggedSequencer<SeqTag,SeqTrigger>;
  using VirtualInfoType = VirtualInfo;

  template <typename MessageT, typename VcT>
  using SeqTriggerType = SeqMigratableVrtTriggerType<MessageT, VcT>;

  template <typename MessageT, typename VcT>
  using SeqActionType = ActionVirtual<MessageT, VcT>;

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  using SeqStateMatcherType = SeqMatcherVirtual<VcT, MsgT, f>;

  SeqType createVirtualSeq(VirtualProxyType const& proxy);
  VirtualProxyType getCurrentVirtualProxy();

  virtual SeqType getNextID() override;

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void sequenceVrtMsg(MsgT* msg, VcT* vrt_context);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void wait(TagType const& tag, SeqTriggerType<MsgT, VcT> trigger);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void wait(SeqTriggerType<MsgT, VcT> trigger);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void wait_on_trigger(TagType const& tag, SeqActionType<MsgT, VcT> action);

private:
  typename Base::template SeqIDContainerType<VirtualInfoType> seq_vrt_lookup_;
};

#define SEQUENCE_REGISTER_VRT_HANDLER(vrtcontext, message, handler)     \
  static void handler(message* m, vrtcontext* vc) {                     \
    theVirtualSeq()->sequenceVrtMsg<vrtcontext, message, handler>(m, vc); \
  }

using SequencerVirtual = TaggedSequencerVrt<SeqType, SeqMigratableTriggerType>;

}} //end namespace vt::seq

namespace vt {

extern seq::SequencerVirtual* theVirtualSeq();

} //end namespace vt

#include "vt/sequence/sequencer_virtual.impl.h"

#endif /*INCLUDED_SEQUENCE_SEQUENCER_VIRTUAL_H*/
