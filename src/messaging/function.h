
#if ! defined __RUNTIME_TRANSPORT_FUNCTION__
#define __RUNTIME_TRANSPORT_FUNCTION__

#include "common.h"
#include "envelope.h"
#include "message.h"

namespace runtime {

using active_function_t = std::function<void(runtime::BaseMessage*)>;
using action_basic_function_t = void(runtime::BaseMessage *);
using simple_function_t = void(*)(runtime::BaseMessage *);

template <typename MessageT>
using action_any_function_t = void(MessageT *);

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_FUNCTION__*/
