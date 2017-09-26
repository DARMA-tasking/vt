
#if !defined INCLUDED_REGISTRY_FUNCTION
#define INCLUDED_REGISTRY_FUNCTION

#include "config.h"
#include "message.h"

namespace vt {

using ActiveFunctionType = std::function<void(vt::BaseMessage*)>;
using ActiveBasicFunctionType = void(vt::BaseMessage *);
using SimpleFunctionType = void(*)(vt::BaseMessage *);

template <typename MessageT>
using ActiveAnyFunctionType = void(MessageT *);

}  // end namespace vt

#endif  /*INCLUDED_REGISTRY_FUNCTION*/
