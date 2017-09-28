
#if !defined INCLUDED_CONTEXT_VRT_FWD
#define INCLUDED_CONTEXT_VRT_FWD

#include "config.h"
#include "message.h"
#include "context_vrt.h"

using SimpleVCFunctionType = void(*)(vt::BaseMessage *, vt::vrt::VrtContext*);

//using ActiveVCBasicFunctionType = void(vt::BaseMessage *);

template <typename MessageT, typename VirtualContextT>
using ActiveVCFunctionType = void(MessageT*, VirtualContextT*);

#endif /*INCLUDED_CONTEXT_VRT_FWD*/

