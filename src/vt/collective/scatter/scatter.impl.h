
#if !defined INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H
#define INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H

#include "vt/config.h"
#include "vt/collective/scatter/scatter.h"
#include "vt/collective/scatter/scatter_msg.h"
#include "vt/context/context.h"

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
  auto remaining_size = thePool()->remainingSize(
    reinterpret_cast<void*>(scatter_msg)
  );
  assert(remaining_size >= combined_size && "Remaining size must be sufficient");
  debug_print(
    scatter, node,
    "Scatter::scatter: total_size={}, elm_size={}, ScatterMsg={}, msg-ptr={}, "
    "ptr={}, remaining_size={}\n",
    total_size, elm_size, sizeof(ScatterMsg), print_ptr(scatter_msg),
    print_ptr(ptr), remaining_size
  );
  auto const& root_node = 0;
  auto nptr = applyScatterRecur(root_node, ptr, elm_size, size_fn, data_fn);
  debug_print(
    scatter, node,
    "Scatter::scatter: incremented size={}\n",
    nptr-ptr
  );
  assert(nptr == ptr + combined_size && "nptr must match size");
  auto const& handler = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
  auto const& this_node = theContext()->getNode();
  scatter_msg->user_han = handler;
  if (this_node != root_node) {
    theMsg()->sendMsgSz<ScatterMsg,scatterHandler>(
      root_node, scatter_msg, sizeof(ScatterMsg) + combined_size
    );
  } else {
    scatterIn(scatter_msg);
  }
}

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H*/
