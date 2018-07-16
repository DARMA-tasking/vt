
#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_NONE_OP_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_NONE_OP_H

#include "config.h"

namespace vt { namespace collective { namespace reduce { namespace operators {

using NoneType = char;

template <typename T>
struct None {
  void operator()(T& v1, T const& v2) {}
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using NoneOp = reduce::operators::None<T>;

using NoneType = reduce::operators::NoneType;

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_NONE_OP_H*/
