
#if !defined INCLUDED_REGISTRY_FUNCTION
#define INCLUDED_REGISTRY_FUNCTION

#include "config.h"
#include "message.h"

namespace vt {

using ActiveClosureFnType = std::function<void(vt::BaseMessage*)>;
using ActiveFnType = void(vt::BaseMessage *);
using ActiveFnPtrType = void(*)(vt::BaseMessage *);

template <typename MessageT>
using ActiveTypedFnType = void(MessageT *);

}  // end namespace vt

#endif  /*INCLUDED_REGISTRY_FUNCTION*/
