
#if ! defined __VIRTUAL_TRANSPORT_TEST_MSG__
#define __VIRTUAL_TRANSPORT_TEST_MSG__

#include "transport.h"

#include <array>
#include <cstdint>

namespace vt { namespace tests { namespace unit {

using NumBytesType = int64_t;
using ByteType = char;

template <typename MessageT, NumBytesType num_bytes>
struct TestStaticBytesMsg : MessageT {
  std::array<ByteType, num_bytes> payload{};
  NumBytesType bytes = 0;

  TestStaticBytesMsg() : MessageT() { }

  explicit TestStaticBytesMsg(NumBytesType const& in_bytes)
    : MessageT(), bytes(in_bytes)
  { }

  explicit TestStaticBytesMsg(
    NumBytesType const& in_bytes, std::array<ByteType, num_bytes>&& arr
  ) : MessageT(), payload(std::forward<std::array<ByteType, num_bytes>>(arr)),
      bytes(in_bytes)
  { }
};

template <NumBytesType num_bytes>
using TestStaticBytesNormalMsg = TestStaticBytesMsg<vt::Message, num_bytes>;

template <NumBytesType num_bytes>
using TestStaticBytesShortMsg = TestStaticBytesMsg<vt::ShortMessage, num_bytes>;

}}} // end namespace vt::tests::unit

#endif /* __VIRTUAL_TRANSPORT_TEST_MSG__ */
