
#if !defined INCLUDED_REGISTRY_FUNCTION
#define INCLUDED_REGISTRY_FUNCTION

#include "config.h"
#include "messaging/message.h"
#include "configs/types/types_rdma.h"

namespace vt {

/*******************************************************************************
 *
 *                          Active Function Naming Scheme
 *
 *******************************************************************************
 *
 *    - All registerable active function handlers start with "Active" and end
 *      with "Type", since they are a type alias.
 *
 *    - If the active function is a pointer, it should end with `FnPtr' rather
 *      than just `Fn'
 *
 *      -- Non-function pointer
 *
 *            using ActiveFnType = void(BaseMessage*);
 *
 *      -- Function pointer
 *
 *            using ActiveFnPtrType = void(*)(BaseMessage*);
 *
 *     - If the active function is typed, it should have `Typed' before the
 *       Fn/FnPtr:
 *
 *      -- Typed function pointer
 *
 *            template <typename MessageT>
 *            using ActiveTypedFnPtrType = void(*)(MessageT*);
 *
 *    - If the active function is a closure (can have state associated with
 *      it beyond just a pointer), it should be post-fixed with `Closure'
 *
 *            using ActiveClosureFnType = std::function<void(BaseMessage*)>;
 *
 *******************************************************************************
 */

using ActiveClosureFnType = std::function<void(vt::BaseMessage*)>;
using ActiveFnType = void(vt::BaseMessage *);
using ActiveFnPtrType = void(*)(vt::BaseMessage *);

template <typename MessageT>
using ActiveTypedFnType = void(MessageT *);

using ActiveClosureRDMAGetFnType = std::function<
  RDMA_GetType(vt::BaseMessage*, ByteType, ByteType, TagType, bool)
>;
using ActiveRDMAGetFnPtrType = RDMA_GetType(*)(
  vt::BaseMessage *, ByteType, ByteType, TagType, bool
);
template <typename MessageT>
using ActiveTypedRDMAGetFnType = RDMA_GetType(
  MessageT*, ByteType, ByteType, TagType, bool
);

using ActiveClosureRDMAPutFnType = std::function<
  void(vt::BaseMessage*, RDMA_PtrType, ByteType, ByteType, TagType, bool)
>;
using ActiveRDMAPutFnPtrType = void(*)(
  vt::BaseMessage *, RDMA_PtrType, ByteType, ByteType, TagType, bool
);
template <typename MessageT>
using ActiveTypedRDMAPutFnType = void(
  MessageT*, RDMA_PtrType, ByteType, ByteType, TagType, bool
);



}  // end namespace vt

#endif  /*INCLUDED_REGISTRY_FUNCTION*/
