
#include "vt/config.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/runtime/runtime.h"

#include <memory>

namespace vt {

//DeclareInitTLS(runtime::Runtime*, curRT, nullptr);
runtime::Runtime* curRT = nullptr;
::vt::runtime::Runtime* rt = nullptr;

} /* end namespace vt */
