
#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_OR_OP_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_OR_OP_H

#include "config.h"

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename T>
struct OrOp {
  void operator()(T& v1, T const& v2) {
    v1 = v1 || v2;
  }
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using OrOp = reduce::operators::OrOp<T>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_OR_OP_H*/
