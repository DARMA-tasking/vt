
#if !defined INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H
#define INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"

namespace vt { namespace auto_registry {

inline AutoActiveCollectionType getAutoHandlerCollection(
  HandlerType const& handler
) {
  using ContainerType = AutoActiveCollectionContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
inline HandlerType makeAutoHandlerCollection(MsgT* const msg) {
  using FunctorT = FunctorAdapter<ActiveColTypedFnType<MsgT, ColT>, f>;
  using ContainerType = AutoActiveCollectionContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionType>;
  using FuncType = ActiveColFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}

inline AutoActiveCollectionMemType getAutoHandlerCollectionMem(
  HandlerType const& handler
) {
  using ContainerType = AutoActiveCollectionMemContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <
  typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
inline HandlerType makeAutoHandlerCollectionMem(MsgT* const msg) {
  using FunctorT = FunctorAdapterMember<ActiveColMemberTypedFnType<MsgT, ColT>, f>;
  using ContainerType = AutoActiveCollectionMemContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionMemType>;
  using FuncType = ActiveColMemberFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}


}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H*/
