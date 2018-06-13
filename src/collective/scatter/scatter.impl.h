
#if !defined INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H
#define INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H

#include "config.h"
#include "collective/scatter/scatter.h"
#include "collective/scatter/scatter_msg.h"
#include "context/context.h"

#include <cassert>
#include <cstring>

namespace vt { namespace collective { namespace scatter {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void Scatter::scatter(
  std::size_t const& total_size, std::size_t const& max_proc_size,
  FuncSizeType size_fn, FuncDataType data_fn
) {
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& elm_size = max_proc_size;
  auto const& combined_size = num_nodes * elm_size;
  auto scatter_msg = makeSharedMessageSz<ScatterMsg>(
    combined_size, combined_size, elm_size
  );
  assert(total_size == combined_size && "Sizes must be consistent");
  auto ptr = reinterpret_cast<char*>(scatter_msg) + sizeof(ScatterMsg);
  debug_print_force(
    scatter, node,
    "Scatter::scatter: total_size={}, elm_size={}, ScatterMsg={}, msg-ptr={}, "
    "ptr={}\n",
    total_size, elm_size, sizeof(ScatterMsg), print_ptr(scatter_msg),
    print_ptr(ptr)
  );
  auto const& root_node = 0;
  auto nptr = applyScatterRecur<>(root_node, ptr, elm_size, size_fn, data_fn);
  assert(nptr == ptr + combined_size && "nptr must match size");
  auto const& handler = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
  auto const& this_node = theContext()->getNode();
  scatter_msg->user_han = handler;
  if (this_node != root_node) {
    theMsg()->sendMsgSz<ScatterMsg,scatterHandler<>>(
      root_node, scatter_msg, sizeof(ScatterMsg) + combined_size
    );
  } else {
    scatterIn(scatter_msg);
  }
}

template <typename>
char* Scatter::applyScatterRecur(
  NodeType node, char* ptr, std::size_t elm_size, FuncSizeType size_fn,
  FuncDataType data_fn
) {
  // pre-order k-ary tree traversal for data layout
  auto children = Tree::getChildren(node);
  char* cur_ptr = ptr;
  data_fn(node, reinterpret_cast<void*>(cur_ptr));
  cur_ptr += elm_size;
  for (auto&& child : children) {
    cur_ptr = applyScatterRecur<>(child, cur_ptr, elm_size, size_fn, data_fn);
    cur_ptr = applyScatterRecur<>(child, cur_ptr, elm_size, size_fn, data_fn);
  }
  return cur_ptr;
}

template <typename>
void Scatter::scatterIn(ScatterMsg* msg) {
  auto const& total_children = getNumTotalChildren();
  auto const& elm_size = msg->elm_bytes_;
  auto const& total_size = msg->total_bytes_;
  auto in_ptr = reinterpret_cast<char*>(msg) + sizeof(ScatterMsg);
  Tree::foreachChild([&](NodeType child) {
    auto const& num_children = getNumTotalChildren(child);
    auto child_msg = makeSharedMessageSz<ScatterMsg>(
      total_children * elm_size, total_size, elm_size
    );
    auto ptr = reinterpret_cast<char*>(child_msg) + sizeof(ScatterMsg);
    auto const& bytes_size = num_children * elm_size;
    std::memcpy(ptr, in_ptr, bytes_size);
    in_ptr += bytes_size;
    theMsg()->sendMsgSz<ScatterMsg,scatterHandler<>>(
      child, child_msg, sizeof(ScatterMsg) + bytes_size
    );
  });
  messageRef(msg);
  auto const& user_handler = msg->user_han;
  auto active_fn = auto_registry::getAutoHandler(user_handler);
  active_fn(reinterpret_cast<BaseMessage*>(in_ptr));
  messageDeref(msg);
}

template <typename>
/*static*/ void Scatter::scatterHandler(ScatterMsg* msg) {
  return theCollective()->scatterIn<>(msg);
}

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H*/
