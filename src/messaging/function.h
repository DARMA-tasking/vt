
#if ! defined __RUNTIME_TRANSPORT_FUNCTION__
#define __RUNTIME_TRANSPORT_FUNCTION__

#include "common.h"
#include "message.h"

namespace runtime {

using ActiveFunctionType = std::function<void(runtime::BaseMessage*)>;
using ActiveBasicFunctionType = void(runtime::BaseMessage *);
using SimpleFunctionType = void(*)(runtime::BaseMessage *);

template <typename MessageT>
using ActiveAnyFunctionType = void(MessageT *);

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_FUNCTION__*/
