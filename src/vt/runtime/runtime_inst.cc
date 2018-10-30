
#include "config.h"
#include "runtime_inst.h"
#include "runtime.h"

#include <memory>

namespace vt {

//DeclareInitTLS(runtime::Runtime*, curRT, nullptr);
runtime::Runtime* curRT = nullptr;
::vt::runtime::Runtime* rt = nullptr;

} /* end namespace vt */
