
#if ! defined __RUNTIME_TRANSPORT_SEQ_COMMON__
#define __RUNTIME_TRANSPORT_SEQ_COMMON__

#include <cstdint>
#include <functional>

#include "function.h"

namespace vt { namespace seq {

using SeqType = int32_t;
using UserSeqFunType = std::function<void()>;
using UserSeqFunWithIDType = std::function<void(SeqType const&)>;

template <typename MessageT>
using SeqNonMigratableTriggerType = std::function<void(MessageT*)>;

template <typename MessageT>
using SeqMigratableTriggerType = ActiveAnyFunctionType<MessageT>;

using SeqCallableType = std::function<bool()>;

static constexpr SeqType const initial_seq = 0;
static SeqType next_seq_id = initial_seq;
static constexpr SeqType const no_seq = -1;

void contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
);

}} //end namespace vt::seq

namespace vt {

using SeqType = seq::SeqType;
using UserSeqFunType = seq::UserSeqFunType;
using UserSeqFunWithIDType = seq::UserSeqFunWithIDType;

} // end namespace vt

#endif /* __RUNTIME_TRANSPORT_SEQ_COMMON__*/
