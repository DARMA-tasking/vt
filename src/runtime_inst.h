
#if !defined INCLUDED_RUNTIME_INST_H
#define INCLUDED_RUNTIME_INST_H

#include "config.h"

#include <memory>

namespace vt { namespace runtime {

struct Runtime;

enum eRuntimeInstance {
  DefaultInstance = 0
};

template <eRuntimeInstance instance = DefaultInstance>
struct RuntimeInst {
  static std::unique_ptr<Runtime> rt;
};

}} /* end namespace vt::runtime */

namespace vt {

extern runtime::Runtime* rt;

} /* end namespace vt */

#include "runtime_inst.impl.h"

#endif /*INCLUDED_RUNTIME_INST_H*/
