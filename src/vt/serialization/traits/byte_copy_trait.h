
#if !defined INCLUDED_SERIALIZATION_TRAITS_BYTE_COPY_TRAIT_H
#define INCLUDED_SERIALIZATION_TRAITS_BYTE_COPY_TRAIT_H

#include "vt/config.h"

#include <type_traits>

namespace vt { namespace serialization {

struct ByteCopyTrait {
  using isByteCopyable = std::true_type;
};

struct NotByteCopyTrait {
  using isByteCopyable = std::false_type;
};

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_TRAITS_BYTE_COPY_TRAIT_H*/
