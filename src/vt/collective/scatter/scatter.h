
#if !defined INCLUDED_COLLECTIVE_SCATTER_SCATTER_H
#define INCLUDED_COLLECTIVE_SCATTER_SCATTER_H

#include "vt/config.h"
#include "vt/collective/scatter/scatter_msg.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/tree/tree.h"

#include <functional>
#include <cstdlib>

namespace vt { namespace collective { namespace scatter {

struct Scatter : virtual collective::tree::Tree {
  using FuncSizeType = std::function<std::size_t(NodeType)>;
  using FuncDataType = std::function<void(NodeType, void*)>;

  Scatter();

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void scatter(
    std::size_t const& total_size, std::size_t const& max_proc_size,
    FuncSizeType size_fn, FuncDataType data_fn
  );

protected:
  void scatterIn(ScatterMsg* msg);

private:
  char* applyScatterRecur(
    NodeType node, char* ptr, std::size_t elm_size, FuncSizeType size_fn,
    FuncDataType data_fn
  );
  static void scatterHandler(ScatterMsg* msg);
};

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_COLLECTIVE_SCATTER_SCATTER_H*/
