
#if !defined INCLUDED_VRT_CONTEXT_MANAGER
#define INCLUDED_VRT_CONTEXT_MANAGER

#include "config.h"
#include "utils/bits/bits_common.h"
#include "configs/types/types_headers.h"

namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContext_BitsType = eVrtContextBits;
  using VrtContext_UniversalIdType = VrtContextType;
  VrtContext_UniversalIdType vrtC_UID;

};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_VRT_CONTEXT_MANAGER*/