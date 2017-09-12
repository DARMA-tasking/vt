
#if ! defined __RUNTIME_TRANSPORT_HANDLER__
#define __RUNTIME_TRANSPORT_HANDLER__

#include <vector>
#include <unordered_map>
#include <cassert>

#include "common.h"
#include "bit_common.h"
#include "function.h"

namespace runtime {

using HandlerIdentifierType = int16_t;

static constexpr HandlerIdentifierType const first_handle_identifier = 1;
static constexpr HandlerIdentifierType const uninitialized_handle_identifier = -1;
static constexpr HandlerType const blank_handler = 0;

static constexpr BitCountType const auto_num_bits = 1;
static constexpr BitCountType const functor_num_bits = 1;
static constexpr BitCountType const handler_id_num_bits =
  BitCounterType<HandlerIdentifierType>::value;

enum eHandlerBits {
  Auto       = 0,
  Functor    = eHandlerBits::Auto       + auto_num_bits,
  Identifier = eHandlerBits::Functor    + functor_num_bits,
  Node       = eHandlerBits::Identifier + handler_id_num_bits,
};

struct HandlerManager {
  using HandlerBitsType = eHandlerBits;

  HandlerManager() = default;

  static HandlerType make_handler(
    bool const& is_auto, bool const& is_functor, HandlerIdentifierType const& id
  );
  static NodeType get_handler_node(HandlerType const& han);
  static void set_handler_node(HandlerType& han, NodeType const& node);
  static void set_handler_identifier(
    HandlerType& han, HandlerIdentifierType const& ident
  );
  static HandlerIdentifierType get_handler_identifier(HandlerType const& han);
  static void set_handler_auto(HandlerType& han, bool const& is_auto);
  static void set_handler_functor(HandlerType& han, bool const& is_functor);
  static bool is_handler_auto(HandlerType const& han);
  static bool is_handler_functor(HandlerType const& han);
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
