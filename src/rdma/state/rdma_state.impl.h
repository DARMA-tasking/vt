
#if !defined INCLUDED_RDMA_RDMA_STATE_IMPL_H
#define INCLUDED_RDMA_RDMA_STATE_IMPL_H

#include "rdma/state/rdma_state.h"

namespace vt { namespace rdma {

template <typename AssocFuncT, typename FuncT>
RDMA_HandlerType State::setRDMAGetFn(
  AssocFuncT* msg, FuncT const& fn, bool const& any_tag, TagType const& tag
) {
  auto const& this_node = theContext()->getNode();

  debug_print(
    rdma_state, node,
    "setRDMAGetFn: GET tag=%d, handle=%lld, any_tag=%s\n",
    tag, handle, print_bool(any_tag)
  );

  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Get);

  if (any_tag) {
    assert(
      tag == no_tag and "If any tag, you must not have a tag set"
    );
  }

  this_rdma_get_handler = handler;
  user_state_get_msg_ = msg;

  auto const& gen_fn = reinterpret_cast<RDMA_GetFunctionType>(fn);

  if (tag == no_tag) {
    rdma_get_fn = gen_fn;
    get_any_tag = any_tag;
  } else {
    get_tag_holder[tag] = RDMA_TagGetHolderType{gen_fn,handler};
  }

  return handler;
}

template <typename AssocFuncT, typename FuncT>
RDMA_HandlerType State::setRDMAPutFn(
  AssocFuncT* msg, FuncT const& fn, bool const& any_tag, TagType const& tag
) {
  RDMA_HandlerType const handler = makeRdmaHandler(RDMA_TypeType::Put);

  auto const& this_node = theContext()->getNode();

  debug_print(
    rdma_state, node,
    "setRDMAPutFn: PUT tag=%d, handle=%lld, any_tag=%s\n",
    tag, handle, print_bool(any_tag)
  );

  if (any_tag) {
    assert(
      tag == no_tag and "If any tag, you must not have a tag set"
    );
  }

  this_rdma_put_handler = handler;
  user_state_put_msg_ = msg;

  auto const& gen_fn = reinterpret_cast<RDMA_PutFunctionType>(fn);

  if (tag == no_tag) {
    rdma_put_fn = gen_fn;
    put_any_tag = any_tag;
  } else {
    put_tag_holder[tag] = RDMA_TagPutHolderType{gen_fn,handler};
  }

  return handler;
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_RDMA_RDMA_STATE_IMPL_H*/
