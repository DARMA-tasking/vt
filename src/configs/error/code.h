
#if !defined INCLUDED_CONFIGS_ERROR_CODE_H
#define INCLUDED_CONFIGS_ERROR_CODE_H

#include <cstdlib>

namespace vt { namespace error {

enum struct ErrorClsCode : int16_t {
  VrtlCollection   = 0,
  VrtlContext      = 1,
  RDMA             = 2,
  Barrier          = 3,
  Reduce           = 4,
  Scatter          = 5,
  Epoch            = 6,
  Event            = 7,
  Group            = 8,
  Handler          = 9,
  LB               = 10,
  Active           = 11,
  Pipe             = 12,
  Pool             = 13,
  Runtime          = 14,
  Timing           = 15,
  Termination      = 16,
  Trace            = 17,
  Serialization    = 18,
  Util             = 19,
  Sequence         = 20,
  Registry         = 21,
  Location         = 22,
  Index            = 23,
  Mapping          = 24,
  Worker           = 25,
  Parameterization = 26
};

enum struct ErrorCode : int16_t {

};

}} /* end namespace vt::error */

#endif /*INCLUDED_CONFIGS_ERROR_CODE_H*/
