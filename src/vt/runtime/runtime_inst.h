
#if !defined INCLUDED_RUNTIME_INST_H
#define INCLUDED_RUNTIME_INST_H

#include "vt/config.h"
#include "vt/runtime/runtime_common.h"
#include "vt/utils/tls/tls.h"

#include <memory>

namespace vt { namespace runtime {

struct Runtime;

template <eRuntimeInstance instance = DefaultInstance>
struct RuntimeInst {
  using RuntimeType = ::vt::runtime::Runtime;
  using RuntimePtrType = std::unique_ptr<RuntimeType>;

  static RuntimePtrType rt;
};

}} /* end namespace vt::runtime */

namespace vt {

// This is the current runtime instance that is active
extern runtime::Runtime* curRT;
extern runtime::Runtime* rt;

} /* end namespace vt */

#include "vt/runtime/runtime_inst.impl.h"

#endif /*INCLUDED_RUNTIME_INST_H*/
