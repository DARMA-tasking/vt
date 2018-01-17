





























#if !defined INCLUDED_REDUCTION_REDUCTION_OPS_H
#define INCLUDED_REDUCTION_REDUCTION_OPS_H

namespace vt { namespace reduction {

template <typename T>
T* plusOp(T* left, T* right) {
  return (*left) + (*right);
}

}}  // end namespace vt::reduction

#endif /*INCLUDED_REDUCTION_REDUCTION_OPS_H*/