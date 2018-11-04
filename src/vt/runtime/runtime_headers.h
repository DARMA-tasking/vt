
#if !defined INCLUDED_RUNTIME_RUNTIME_HEADERS_H
#define INCLUDED_RUNTIME_RUNTIME_HEADERS_H

#include "vt/runtime/runtime_common.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/runtime/runtime.h"
#include "vt/runtime/runtime_get.h"
#include "vt/runtime/runtime_holder.h"

namespace vt { namespace runtime {

using RuntimeHolderType = RuntimeHolder;
using RuntimeHolderUnsafePtrType = RuntimeHolderType::PointerType;

}} /* end namespace vt::runtime */

/*
 * The runtime types exposed to other components that interact with the runtime
 * access it through the holder.
 */
namespace vt {

using RuntimeType = runtime::RuntimeHolderType;
using RuntimeUnsafePtrType = runtime::RuntimeHolderUnsafePtrType;
using RuntimePtrType = RuntimeType;

} /* end namespace vt */

#endif /*INCLUDED_RUNTIME_RUNTIME_HEADERS_H*/
