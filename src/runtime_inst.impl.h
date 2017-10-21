
#if !defined INCLUDED_RUNTIME_INST_IMPL_H
#define INCLUDED_RUNTIME_INST_IMPL_H

#include "config.h"
#include "runtime_inst.h"
#include "runtime.h"

#include <memory>

namespace vt { namespace runtime {

template <eRuntimeInstance instance>
/*static*/ std::unique_ptr<Runtime> RuntimeInst<instance>::rt = nullptr;

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_INST_IMPL_H*/
