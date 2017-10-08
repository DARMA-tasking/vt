
#if ! defined __RUNTIME_TRANSPORT_HANDLER__
#define __RUNTIME_TRANSPORT_HANDLER__

#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdint>

#include "config.h"
#include "utils/bits/bits_common.h"
#include "activefn/activefn.h"

namespace vt {

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

  static HandlerType makeHandler(
    bool const& is_auto, bool const& is_functor, HandlerIdentifierType const& id
  );
  static NodeType getHandlerNode(HandlerType const& han);
  static void setHandlerNode(HandlerType& han, NodeType const& node);
  static void setHandlerIdentifier(
    HandlerType& han, HandlerIdentifierType const& ident
  );
  static HandlerIdentifierType getHandlerIdentifier(HandlerType const& han);
  static void setHandlerAuto(HandlerType& han, bool const& is_auto);
  static void setHandlerFunctor(HandlerType& han, bool const& is_functor);
  static bool isHandlerAuto(HandlerType const& han);
  static bool isHandlerFunctor(HandlerType const& han);
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
