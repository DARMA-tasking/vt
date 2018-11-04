
#if !defined INCLUDED_SERIALIZATION_SERIALIZATION_H
#define INCLUDED_SERIALIZATION_SERIALIZATION_H

#include "vt/config.h"
#include "vt/serialization/serialize_interface.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/serialization/messaging/serialized_param_messenger.h"
#include "vt/serialization/traits/byte_copy_trait.h"

namespace vt { namespace serialization {

using SerialByteType = ::serialization::interface::SerialByteType;

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_SERIALIZATION_H*/

