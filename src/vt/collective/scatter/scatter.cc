
#include "vt/config.h"
#include "vt/collective/scatter/scatter.h"
#include "vt/collective/collective_alg.h"
#include "vt/messaging/active.h"

namespace vt { namespace collective { namespace scatter {

Scatter::Scatter()
  : tree::Tree(tree::tree_cons_tag_t)
{ }

char* Scatter::applyScatterRecur(
  NodeType node, char* ptr, std::size_t elm_size, FuncSizeType size_fn,
  FuncDataType data_fn
) {
  // pre-order k-ary tree traversal for data layout
  auto children = Tree::getChildren(node);
  char* cur_ptr = ptr;
  debug_print(
    scatter, node,
    "Scatter::applyScatterRecur: elm_size={}, ptr={}, node={}\n",
    elm_size, print_ptr(ptr), node
  );
  data_fn(node, reinterpret_cast<void*>(cur_ptr));
  cur_ptr += elm_size;
  for (auto&& child : children) {
    debug_print(
      scatter, node,
      "Scatter::applyScatterRecur: child={}\n", child
    );
    cur_ptr = applyScatterRecur(child, cur_ptr, elm_size, size_fn, data_fn);
  }
  return cur_ptr;
}

void Scatter::scatterIn(ScatterMsg* msg) {
  auto const& total_children = getNumTotalChildren();
  auto const& elm_size = msg->elm_bytes_;
  auto const& total_size = msg->total_bytes_;
  auto in_base_ptr = reinterpret_cast<char*>(msg) + sizeof(ScatterMsg);
  auto in_ptr = in_base_ptr + elm_size;
  auto const& user_handler = msg->user_han;
  debug_print(
    scatter, node,
    "Scatter::scatterIn: handler={}, total_size={}, elm_size={}, offset={}, "
    "parent children={}\n",
    user_handler, total_size, elm_size, in_ptr - in_base_ptr, total_children
  );
  Tree::foreachChild([&](NodeType child) {
    auto const& num_children = getNumTotalChildren(child) + 1;
    auto const& child_bytes_size = num_children * elm_size;
    auto child_msg = makeSharedMessageSz<ScatterMsg>(
      child_bytes_size, child_bytes_size, elm_size
    );
    debug_print(
      scatter, node,
      "Scatter::scatterIn: child={}, num_children={}, child_bytes_size={}\n",
      child, num_children, child_bytes_size
    );
    auto const child_remaining_size = thePool()->remainingSize(
      reinterpret_cast<void*>(child_msg)
    );
    child_msg->user_han = user_handler;
    auto ptr = reinterpret_cast<char*>(child_msg) + sizeof(ScatterMsg);
    debug_print(
      scatter, node,
      "Scatter::scatterIn: child={}, num_children={}, elm_size={}, "
      "offset={}, child_remaining={}, parent size={}, child_bytes_size={}\n",
      child, num_children, elm_size, in_ptr - in_base_ptr,
      child_remaining_size, total_size, child_bytes_size
    );
    std::memcpy(ptr, in_ptr, child_bytes_size);
    in_ptr += child_bytes_size;
    theMsg()->sendMsgSz<ScatterMsg,scatterHandler>(
      child, child_msg, sizeof(ScatterMsg) + child_bytes_size
    );
  });
  messageRef(msg);
  auto active_fn = auto_registry::getAutoHandler(user_handler);
  active_fn(reinterpret_cast<BaseMessage*>(in_base_ptr));
  messageDeref(msg);
}

/*static*/ void Scatter::scatterHandler(ScatterMsg* msg) {
  return theCollective()->scatterIn(msg);
}

}}} /* end namespace vt::collective::scatter */
