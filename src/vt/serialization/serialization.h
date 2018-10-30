
#if !defined INCLUDED_SERIALIZATION_SERIALIZATION_H
#define INCLUDED_SERIALIZATION_SERIALIZATION_H

#include "config.h"
#include "serialization/serialize_interface.h"
#include "serialization/messaging/serialized_messenger.h"
#include "serialization/messaging/serialized_param_messenger.h"
#include "serialization/traits/byte_copy_trait.h"

namespace vt { namespace serialization {

using SerialByteType = ::serialization::interface::SerialByteType;

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_SERIALIZATION_H*/

