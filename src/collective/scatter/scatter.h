
#if !defined INCLUDED_COLLECTIVE_SCATTER_SCATTER_H
#define INCLUDED_COLLECTIVE_SCATTER_SCATTER_H

#include "config.h"
#include "collective/scatter/scatter_msg.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "collective/tree/tree.h"

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
  template <typename=void>
  char* applyScatterRecur(
    NodeType node, char* ptr, std::size_t elm_size, FuncSizeType size_fn,
    FuncDataType data_fn
  );
  static void scatterHandler(ScatterMsg* msg);
};

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_COLLECTIVE_SCATTER_SCATTER_H*/
