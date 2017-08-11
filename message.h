
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"

namespace runtime {

// template <typename T>
// struct Message {
//   using envelope_t = Envelope;
//   using user_type_t = T;

//   envelope_t env;
//   user_type_t user_data;

//   Message() = default;

//   Message(
//     envelope_t&& in_env, user_type_t&& in_user_data
//   ) : env(std::move(in_env)), user_data(std::move(in_user_data))
//   { }
// };

struct Message {
  using envelope_t = Envelope;

  envelope_t env;

  Message() = default;
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
