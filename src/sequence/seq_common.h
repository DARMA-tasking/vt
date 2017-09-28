
#if ! defined __RUNTIME_TRANSPORT_SEQ_COMMON__
#define __RUNTIME_TRANSPORT_SEQ_COMMON__

#include <cstdint>
#include <functional>
#include <vector>

#include "config.h"
#include "registry_function.h"

namespace vt { namespace seq {

using SeqType = int32_t;
using UserSeqFunType = std::function<void()>;
using FuncType = UserSeqFunType;
using SystemSeqFunType = std::function<bool()>;
using UserSeqFunWithIDType = std::function<void(SeqType const&)>;
using FuncIDType = UserSeqFunWithIDType;
using SeqFuncContainerType = std::vector<FuncType>;
using ForIndex = int32_t;
using UserSeqFunIndexType = std::function<void(ForIndex idx)>;
using FuncIndexType = UserSeqFunIndexType;

template <typename MessageT>
using SeqNonMigratableTriggerType = std::function<void(MessageT*)>;

template <typename MessageT>
using SeqMigratableTriggerType = ActiveAnyFunctionType<MessageT>;

using SeqContinuation = std::function<void()>;

enum class eSeqConstructType : int8_t {
  WaitConstruct = 1,
  ParallelConstruct = 2,
  InvalidConstruct = -1
};

#define PRINT_SEQ_CONSTRUCT_TYPE(NODE)                                  \
  ((NODE) == eSeqConstructType::WaitConstruct ?"WaitConstruct" :        \
   ((NODE) == eSeqConstructType::ParallelConstruct ? "ParallelConstruct" : \
    ((NODE) == eSeqConstructType::InvalidConstruct ? "InvalidConstruct" : "???") \
   )                                                                    \
  )

using SeqCallableType = std::function<bool()>;

static constexpr SeqType const initial_seq = 0;
static SeqType next_seq_id = initial_seq;
static constexpr SeqType const no_seq = -1;

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
);
void enqueue_action(ActionType const& action);

static constexpr bool const seq_skip_queue = false;

}} //end namespace vt::seq

namespace vt {

using SeqType = seq::SeqType;
using UserSeqFunType = seq::UserSeqFunType;
using UserSeqFunWithIDType = seq::UserSeqFunWithIDType;

} // end namespace vt

#endif /* __RUNTIME_TRANSPORT_SEQ_COMMON__*/
