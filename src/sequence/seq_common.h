
#if ! defined __RUNTIME_TRANSPORT_SEQ_COMMON__
#define __RUNTIME_TRANSPORT_SEQ_COMMON__

namespace runtime { namespace seq {

using seq_t = int32_t;
using user_seq_fun_t = std::function<void()>;
using user_seq_fun_with_id_t = std::function<void(seq_t const&)>;

template <typename MessageT>
using seq_non_migratable_trigger_t = std::function<void(MessageT*)>;

template <typename MessageT>
using seq_migratable_trigger_t = action_any_function_t<MessageT>;

using seq_callable_t = std::function<bool()>;

static constexpr seq_t const initial_seq = 0;
static seq_t next_seq_id = initial_seq;
static constexpr seq_t const no_seq = -1;

void
contextual_execution(
  seq_t const& seq, bool const& is_sequenced, seq_callable_t&& callable
);

}} //end namespace runtime::seq

namespace runtime {

using seq_t = seq::seq_t;
using user_seq_fun_t = seq::user_seq_fun_t;
using user_seq_fun_with_id_t = seq::user_seq_fun_with_id_t;

} // end namespace runtime

#endif /* __RUNTIME_TRANSPORT_SEQ_COMMON__*/
