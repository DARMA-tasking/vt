
#if ! defined __RUNTIME_TRANSPORT_ARCHIVE__
#define __RUNTIME_TRANSPORT_ARCHIVE__

#include "config.h"

#include <cstdint>
#include <cassert>

namespace vt { namespace serialization {

struct SimpleArchive {
  using SerializationModeType = eSerializationMode;
  using SizeType = size_t;

  bool isSizing() const {
    return mode == SerializationModeType::Sizing;
  }

  bool isPacking() const {
    return mode == SerializationModeType::Packing;
  }

  bool isUnpacking() const {
    return mode == SerializationModeType::Unpacking;
  }

  void startSizing() {
    mode = SerializationModeType::Sizing;
    assert(size == 0);
  }

  void startPacking() {
    mode = SerializationModeType::Packing;
    assert(start == cur and start != nullptr and cur != nullptr)
  }

  template <typename T>
  void pack(T val) { }

  void addSize(SizeType const& in_size) {
    size += in_size;
  }

  void allocate() {
    start = cur = new char[size];
  }

  void deallocate() {
    delete [] start;
    start = nullptr;
    cur = nullptr;
  }

private:
  SizeType size = 0;
  SerializationModeType mode = SerializationModeType::Invalid;
  SerialByteType* start = nullptr;
  SerialByteType* cur = nullptr;
};

}} //end namespace vt::serialization

#endif /*__RUNTIME_TRANSPORT_ARCHIVE__*/
