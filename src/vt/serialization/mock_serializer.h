
#if !defined INCLUDED_SERIALIZATION_MOCK_SERIALIZER_H
#define INCLUDED_SERIALIZATION_MOCK_SERIALIZER_H

#include "config.h"
#include "utils/static_checks/all_true.h"

#include <cstdlib>
#include <functional>
#include <cstring>
#include <cassert>
#include <type_traits>
#include <vector>
#include <tuple>

/*
 * Mock serializer that implements the serialization interface for
 * byte-serializable types (std::is_arithmetic and std::tuple) by simply calling
 * std::memcpy on the type.
*/

#if !HAS_SERIALIZATION_LIBRARY

namespace vt { namespace serialization {

using namespace ::serialization::interface;
using namespace ::vt::util;

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

template <typename T>
struct isTuple : std::false_type { };
template <typename... Args>
struct isTuple<std::tuple<Args...>> : std::true_type { };

template <typename T>
struct isVector : std::false_type { };
template <typename U>
struct isVector<std::vector<U>> : std::true_type { };

template <typename T>
struct hasByteCopyTrait {
  template <typename C, typename = typename C::isByteCopyable>
  static typename C::isByteCopyable test(int);

  template <typename C>
  static std::false_type test(...);

  static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T>
struct SerializationTraits {

  template <typename U>
  using arith = typename std::is_arithmetic<U>;

  template <typename U>
  using tuple = isTuple<typename std::decay<U>::type>;

  template <typename U>
  using vector = isVector<typename std::decay<U>::type>;

  static constexpr auto const is_byte_cp =
    arith<T>::value or hasByteCopyTrait<T>::value;
  static constexpr auto const is_byte_cp_tuple = is_byte_cp or tuple<T>::value;

  static constexpr auto const is_not_byte_cp =
    not tuple<T>::value and
    not arith<T>::value and
    not hasByteCopyTrait<T>::value and
    not vector<T>::value;

  static constexpr auto const is_tuple = tuple<T>::value and not is_byte_cp;
  static constexpr auto const is_vec = vector<T>::value and not is_byte_cp;
};


template <typename BufferT, typename T>
struct ByteCopyableTraits {
  using BufferPtrType = std::unique_ptr<BufferT>;

  template <typename U>
  using isByteCopyType =
  typename std::enable_if<SerializationTraits<U>::is_byte_cp, T>::type;

  template <typename U>
  using isVecType =
  typename std::enable_if<SerializationTraits<U>::is_vec, T>::type;

  template <typename U>
  using isTupleType =
  typename std::enable_if<SerializationTraits<U>::is_tuple, T>::type;

  template <typename U>
  using isNotByteCopyType =
  typename std::enable_if<SerializationTraits<U>::is_not_byte_cp, T>::type;

  template <typename... Vs>
  static void tupleStaticCheck(std::tuple<Vs...>& tup) {
    // @todo: do recursive check of tuple for byte-copyability
    using cond = all_true<SerializationTraits<Vs>::is_byte_cp_tuple...>;

    static_assert(
      std::is_same<typename cond::type,std::true_type>::value == true,
      "All tuple elms must be byte copyable"
    );
  }

  template <typename U = T>
  static BufferPtrType apply(T* val, SizeType num, isByteCopyType<U>* x = nullptr) {
    return serializeByte<T, BufferT>(val);
  }

  // Hack to make mock serializer work with vectors
  template <typename U = T>
  static BufferPtrType apply(T* val, SizeType num, isVecType<U>* x = nullptr) {
    using ValueType = typename T::value_type;
    return serializeByte<ValueType, BufferT>(
      &(*val)[0], true, val->size()*sizeof(typename T::value_type)
    );
  }

  // Specialize std::tuple type so that the elements can be checked for
  // byte-wise serializability
  template <typename U = T>
  static BufferPtrType apply(T* val, SizeType num, isTupleType<U>* x = nullptr) {
    //tupleStaticCheck(*val);
    return serializeByte<T, BufferT>(val);
  }

  template <typename U = T>
  static BufferPtrType apply(T* val, SizeType num, isNotByteCopyType<U>* x = nullptr) {
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
    std::memcpy(static_cast<void*>(&val), buf, sizeof(T));
  }
};

}} /* end namespace vt::serialization */

namespace serialization { namespace interface {

using namespace ::vt::serialization;

template <typename T>
SerializedReturnType serialize(T& target, BufferCallbackType fn) {
  auto buf = ByteCopyableTraits<ByteSerializedBuffer, T>::apply(&target, 1);
  if (fn != nullptr) {
    auto user_buf = fn(buf->getSize());
    std::memcpy(user_buf, buf->getBuffer(), buf->getSize());
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

#endif /*INCLUDED_SERIALIZATION_MOCK_SERIALIZER_H*/
