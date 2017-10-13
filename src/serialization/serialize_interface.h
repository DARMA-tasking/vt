
#if !defined __RUNTIME_TRANSPORT_SERIALIZE_INTERFACE__
#define __RUNTIME_TRANSPORT_SERIALIZE_INTERFACE__

#include <cstdlib>
#include <functional>
#include <memory>

#if HAS_SERIALIZATION_LIBRARY
  #include "serialization_library_headers.h"
#else

namespace serialization { namespace interface {

using SizeType = size_t;
using SerialByteType = char;

using BufferCallbackType = std::function<SerialByteType*(SizeType size)>;

struct SerializedInfo {
  virtual SizeType getSize() const = 0;
  virtual SerialByteType* getBuffer() const = 0;
  virtual ~SerializedInfo() { }
};

using SerializedInfoPtrType = std::unique_ptr<SerializedInfo>;
using SerializedReturnType = SerializedInfoPtrType;

template <typename T>
SerializedReturnType serialize(T& target, BufferCallbackType fn = nullptr);

template <typename T>
T* deserialize(SerialByteType* buf, SizeType size, T* user_buf = nullptr);

}} /* end namespace serialization::interface */

#include "serialize_interface.impl.h"

#endif

#endif /*__RUNTIME_TRANSPORT_SERIALIZE_INTERFACE__*/

