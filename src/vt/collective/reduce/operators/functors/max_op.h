
#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_MAX_OP_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_MAX_OP_H

#include "vt/config.h"

#include <algorithm>

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename T>
struct MaxOp {
  void operator()(T& v1, T const& v2) {
    v1 = std::max(v1,v2);
  }
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using MaxOp = reduce::operators::MaxOp<T>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_MAX_OP_H*/
