
#if !defined __RUNTIME_TRANSPORT_MOCK_SERIALIZER__
#define __RUNTIME_TRANSPORT_MOCK_SERIALIZER__

#include "config.h"

#include <cstdlib>
#include <functional>
#include <cstring>
#include <cassert>

/*
 * Mock serializer that implements the serialization interface for
 * byte-serializable types (std::is_arithmetic and std::tuple) by simply calling
 * std::memcpy on the type.
*/

#if !HAS_SERIALIZATION_LIBRARY

namespace vt { namespace serialization {

using namespace ::serialization::interface;

struct ByteSerializedBuffer : SerializedInfo {
  using ByteType = SerialByteType[];

  ByteSerializedBuffer(SizeType const& size)
    : size_(size), buffer_(std::make_unique<ByteType>(size))
  { }

  virtual SerialByteType* getBuffer() const override {
    return buffer_.get();
  }

  virtual SizeType getSize() const override {
    return size_;
  }

private:
  SizeType size_ = 0;

  std::unique_ptr<ByteType> buffer_ = nullptr;
};

using ByteSerializedBufferPtrType = std::unique_ptr<ByteSerializedBuffer>;
using BufferPtrType = ByteSerializedBufferPtrType;

template <typename T, typename BufferT>
static inline std::unique_ptr<BufferT> serializeByte(
  T* target, bool hasSize = false, SizeType inSize = 0
) {
  using PtrType = std::unique_ptr<BufferT>;

  SizeType size = 0;
  PtrType buf = nullptr;

  auto tptr = reinterpret_cast<SerialByteType*>(target);

  if (hasSize) {
    size = inSize + sizeof(SizeType);
    buf = std::make_unique<BufferT>(size);
    SerialByteType* cur = buf->getBuffer();
    std::memcpy(cur, reinterpret_cast<SerialByteType*>(&inSize), sizeof(SizeType));
    std::memcpy(cur + sizeof(SizeType), tptr, inSize);
  } else {
    size = sizeof(T);
    buf = std::make_unique<BufferT>(size);
    std::memcpy(buf->getBuffer(), tptr, size);
  }

  return buf;
}

template<typename T>
struct isTuple : std::false_type { };
template<typename... Args>
struct isTuple<std::tuple<Args...>> : std::true_type { };

template<typename T>
struct isVector : std::false_type { };
template<typename U>
struct isVector<std::vector<U>> : std::true_type { };

template <typename T>
struct SerializationTraits {

  template <typename U>
  using byte_copy = typename std::is_arithmetic<U>;

  // template <typename U>
  // using byte_copy_trait = typename std::is_arithmetic<U::byte_copy>;

  template <typename U>
  using is_tuple = isTuple<typename std::decay<U>::type>;

  template <typename U>
  using is_vector = isVector<typename std::decay<U>::type>;

  static constexpr auto const is_byte_copyable =
    is_tuple<T>::value or byte_copy<T>::value;

  static constexpr auto const is_not_byte_copyable =
    not is_tuple<T>::value and
    not byte_copy<T>::value and
    not is_vector<T>::value;

  static constexpr auto const is_vec =
    is_vector<T>::value and not byte_copy<T>::value;
};


template <typename T, typename BufferT>
struct ByteCopyableTraits {
  using BufferPtrType = std::unique_ptr<BufferT>;

  template <typename U>
  using isByteCopyType =
  typename std::enable_if<SerializationTraits<U>::is_byte_copyable, T>::type;

  template <typename U>
  using isVecType =
  typename std::enable_if<SerializationTraits<U>::is_vec, T>::type;

  template <typename U>
  using isNotByteCopyType =
  typename std::enable_if<SerializationTraits<U>::is_not_byte_copyable, T>::type;

  template <typename U = T>
  BufferPtrType operator()(T* val, SizeType num, isByteCopyType<U>* x = nullptr) {
    return serializeByte<T, BufferT>(val);
  }

  // Hack to make mock serializer work with vectors
  template <typename U = T>
  BufferPtrType operator()(T* val, SizeType num, isVecType<U>* x = nullptr) {
    using ValueType = typename T::value_type;
    return serializeByte<ValueType, BufferT>(
      &(*val)[0], true, val->size()*sizeof(typename T::value_type)
    );
  }

  template <typename U = T>
  BufferPtrType operator()(T* val, SizeType num, isNotByteCopyType<U>* x = nullptr) {
    assert(false and "Non-byte copyable serialization not implemented");
  }
};

template <typename T>
struct DeserTraits {
  template <typename U>
  using isVecType = typename std::enable_if<SerializationTraits<U>::is_vec, T>::type;

  template <typename U>
  using isNotVecType = typename std::enable_if<!SerializationTraits<U>::is_vec, T>::type;

  template <typename U = T>
  void operator()(SerialByteType* buf, T& vec, SizeType size, isVecType<U>* x = nullptr) {
    SizeType* buf_as_size = reinterpret_cast<SizeType*>(buf);
    SizeType vec_bytes_size = *buf_as_size;
    vec.resize(vec_bytes_size / sizeof(typename T::value_type));
    std::memcpy(&vec[0], buf_as_size+1, vec_bytes_size);
  }

  template <typename U = T>
  void operator()(SerialByteType* buf, T& val, SizeType num, isNotVecType<U>* x = nullptr) {
    std::memcpy(&val, buf, sizeof(T));
  }
};

}} /* end namespace vt::serialization */

namespace serialization { namespace interface {

using namespace ::vt::serialization;

template <typename T>
SerializedReturnType serialize(T& target, BufferCallbackType fn) {
  auto const& size = sizeof(T);
  ByteCopyableTraits<T, ByteSerializedBuffer> byte;
  auto buf = byte(&target, 1);
  if (fn != nullptr) {
    auto user_buf = fn(size);
    std::memcpy(user_buf, buf->getBuffer(), size);
  }
  std::unique_ptr<SerializedInfo> base_ptr(
    static_cast<SerializedInfo*>(buf.release())
  );
  return base_ptr;
}

template <typename T>
T* deserialize(SerialByteType* buf, SizeType size, T* allocBuf) {
  SerialByteType* mem = allocBuf ?
    reinterpret_cast<SerialByteType*>(allocBuf) : new SerialByteType[sizeof(T)];
  T* t_ptr = new (mem) T{};

  DeserTraits<T> ap;
  ap(buf, *t_ptr, size);

  return t_ptr;
}

}} /* end namespace serialization::interface */

#endif

#endif /*__RUNTIME_TRANSPORT_MOCK_SERIALIZER__*/
