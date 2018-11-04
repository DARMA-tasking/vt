
#if !defined INCLUDED_RUNTIME_INST_IMPL_H
#define INCLUDED_RUNTIME_INST_IMPL_H

#include "vt/config.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/runtime/runtime.h"

#include <memory>

namespace vt { namespace runtime {

template <eRuntimeInstance instance>
/*static*/ typename RuntimeInst<instance>::RuntimePtrType RuntimeInst<instance>::rt = nullptr;

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_INST_IMPL_H*/
