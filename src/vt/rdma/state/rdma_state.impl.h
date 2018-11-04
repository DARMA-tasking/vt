
#if !defined INCLUDED_RDMA_RDMA_STATE_IMPL_H
#define INCLUDED_RDMA_RDMA_STATE_IMPL_H

#include "vt/config.h"
#include "vt/rdma/state/rdma_state.h"
#include "vt/messaging/message.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"
#include "vt/registry/auto/rdma/auto_registry_rdma.h"

namespace vt { namespace rdma {

template <typename MsgT, typename FuncT, ActiveTypedRDMAGetFnType<MsgT>* f>
RDMA_HandlerType State::setRDMAGetFn(
  MsgT* msg, FuncT const& fn, bool const& any_tag, TagType const& tag
) {
  auto const& this_node = theContext()->getNode();

  debug_print(
    rdma_state, node,
    "setRDMAGetFn: GET tag={}, handle={}, any_tag={}\n",
    tag, handle, print_bool(any_tag)
  );

  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Get);

  auto const reg_han = auto_registry::makeAutoHandlerRDMAGet<MsgT,f>(nullptr);

  if (any_tag) {
    vtAssert(
      tag == no_tag, "If any tag, you must not have a tag set"
    );
  }

  this_get_handler = reg_han;
  this_rdma_get_handler = handler;
  user_state_get_msg_ = msg;

  auto const& gen_fn = reinterpret_cast<RDMA_GetFunctionType>(fn);

  if (tag == no_tag) {
    rdma_get_fn = gen_fn;
    get_any_tag = any_tag;
  } else {
    get_tag_holder[tag] = RDMA_TagGetHolderType{gen_fn,handler,reg_han};
  }

  return handler;
}

template <typename MsgT, typename FuncT, ActiveTypedRDMAPutFnType<MsgT>* f>
RDMA_HandlerType State::setRDMAPutFn(
  MsgT* msg, FuncT const& fn, bool const& any_tag, TagType const& tag
) {
  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Put);

  auto const& this_node = theContext()->getNode();

  debug_print(
    rdma_state, node,
    "setRDMAPutFn: PUT tag={}, handle={}, any_tag={}\n",
    tag, handle, print_bool(any_tag)
  );

  auto const reg_han = auto_registry::makeAutoHandlerRDMAPut<MsgT,f>(nullptr);

  if (any_tag) {
    vtAssert(
      tag == no_tag, "If any tag, you must not have a tag set"
    );
  }

  this_put_handler = reg_han;
  this_rdma_put_handler = handler;
  user_state_put_msg_ = msg;

  auto const& gen_fn = reinterpret_cast<RDMA_PutFunctionType>(fn);

  if (tag == no_tag) {
    rdma_put_fn = gen_fn;
    put_any_tag = any_tag;
  } else {
    put_tag_holder[tag] = RDMA_TagPutHolderType{gen_fn,handler,reg_han};
  }

  return handler;
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_RDMA_RDMA_STATE_IMPL_H*/
