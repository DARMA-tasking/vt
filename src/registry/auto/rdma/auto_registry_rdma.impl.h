
#if !defined INCLUDED_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_IMPL_H
#define INCLUDED_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_IMPL_H

#include "config.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry_general.h"
#include "registry/auto/rdma/auto_registry_rdma.h"

namespace vt { namespace auto_registry {

inline AutoActiveRDMAPutType getAutoHandlerRDMAPut(HandlerType const& handler) {
  using ContainerType = AutoActiveRDMAPutContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <typename MsgT, ActiveTypedRDMAPutFnType<MsgT>* f>
inline HandlerType makeAutoHandlerRDMAPut(MsgT* const msg) {
  using FunctorT = FunctorAdapter<ActiveTypedRDMAPutFnType<MsgT>, f>;
  using ContainerType = AutoActiveRDMAPutContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveRDMAPutType>;
  using FuncType = ActiveRDMAPutFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}

inline AutoActiveRDMAGetType getAutoHandlerRDMAGet(HandlerType const& handler) {
  using ContainerType = AutoActiveRDMAGetContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <typename MsgT, ActiveTypedRDMAGetFnType<MsgT>* f>
inline HandlerType makeAutoHandlerRDMAGet(MsgT* const msg) {
  using FunctorT = FunctorAdapter<ActiveTypedRDMAGetFnType<MsgT>, f>;
  using ContainerType = AutoActiveRDMAGetContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveRDMAGetType>;
  using FuncType = ActiveRDMAGetFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}


}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_IMPL_H*/
