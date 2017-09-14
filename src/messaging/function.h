
#if ! defined __RUNTIME_TRANSPORT_FUNCTION__
#define __RUNTIME_TRANSPORT_FUNCTION__

#include "config.h"
#include "message.h"

namespace vt {

using ActiveFunctionType = std::function<void(vt::BaseMessage*)>;
using ActiveBasicFunctionType = void(vt::BaseMessage *);
using SimpleFunctionType = void(*)(vt::BaseMessage *);

template <typename MessageT>
using ActiveAnyFunctionType = void(MessageT *);

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_FUNCTION__*/
