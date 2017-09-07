
#if ! defined __RUNTIME_TRANSPORT_HANDLER__
#define __RUNTIME_TRANSPORT_HANDLER__

#include <vector>
#include <unordered_map>
#include <cassert>

#include "common.h"
#include "bit_common.h"
#include "function.h"

namespace runtime {

using handler_identifier_t = int16_t;

static constexpr handler_identifier_t const first_handle_identifier = 1;
static constexpr handler_identifier_t const uninitialized_handle_identifier = -1;
static constexpr handler_t const blank_handler = 0;

static constexpr bit_count_t const auto_num_bits = 1;
static constexpr bit_count_t const functor_num_bits = 1;
static constexpr bit_count_t const handler_id_num_bits =
  bit_counter_t<handler_identifier_t>::value;

enum HandlerBits {
  Auto       = 0,
  Functor    = HandlerBits::Auto       + auto_num_bits,
  Identifier = HandlerBits::Functor    + functor_num_bits,
  Node       = HandlerBits::Identifier + handler_id_num_bits,
};

struct HandlerManager {
  using handler_bits_t = HandlerBits;

  HandlerManager() = default;

  static handler_t
  make_handler(
    bool const& is_auto, bool const& is_functor, handler_identifier_t const& id
  );

  static node_t
  get_handler_node(handler_t const& han);

  static void
  set_handler_node(handler_t& han, node_t const& node);

  static void
  set_handler_identifier(handler_t& han, handler_identifier_t const& ident);

  static handler_identifier_t
  get_handler_identifier(handler_t const& han);

  static void
  set_handler_auto(handler_t& han, bool const& is_auto);

  static void
  set_handler_functor(handler_t& han, bool const& is_functor);

  static bool
  is_handler_auto(handler_t const& han);

  static bool
  is_handler_functor(handler_t const& han);
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
