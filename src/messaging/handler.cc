
#include "handler.h"
#include "transport.h"

namespace runtime {

/*static*/ handler_t
HandlerManager::make_handler(bool const& is_auto, handler_identifier_t const& id) {
  handler_t new_han = blank_handler;
  HandlerManager::set_handler_auto(new_han, true);
  HandlerManager::set_handler_identifier(new_han, id);
  return new_han;
}

/*static*/ node_t
HandlerManager::get_handler_node(handler_t const& han) {
  node_t const node = static_cast<node_t>(han >> handler_bits_t::Node);
  return node;
}

/*static*/ handler_identifier_t
HandlerManager::get_handler_identifier(handler_t const& han) {
  handler_identifier_t const id = static_cast<handler_identifier_t>(
    han >> handler_bits_t::Identifier
  );
  return id;
}

/*static*/ void
HandlerManager::set_handler_node(handler_t& han, node_t const& node) {
  han |= node << handler_bits_t::Node;
}

/*static*/ void
HandlerManager::set_handler_identifier(
  handler_t& han, handler_identifier_t const& ident
) {
  han |= ident << handler_bits_t::Identifier;
}

/*static*/ void
HandlerManager::set_handler_auto(handler_t& han, bool const& is_auto) {
  if (is_auto) {
    han |= 1 << handler_bits_t::Auto;
  } else {
    han &= 0xFFFFFFFE;
  }
}

/*static*/ bool
HandlerManager::is_handler_auto(handler_t const& han) {
  return han & 0x00000001;
}

} // end namespace runtime
