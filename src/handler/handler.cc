
#include "handler.h"

namespace runtime {

/*static*/ handler_t
HandlerManager::make_handler(
  bool const& is_auto, bool const& is_functor, handler_identifier_t const& id
) {
  handler_t new_han = blank_handler;
  HandlerManager::set_handler_auto(new_han, is_auto);
  HandlerManager::set_handler_functor(new_han, is_functor);
  HandlerManager::set_handler_identifier(new_han, id);

  debug_print_handler(
    "HandlerManager::make_handler: is_functor=%s, is_auto=%s, id=%d, han=%d\n",
    print_bool(is_functor), print_bool(is_auto), id, new_han
  );

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
    han &= ~(1 << handler_bits_t::Auto);
  }
}

/*static*/ void
HandlerManager::set_handler_functor(handler_t& han, bool const& is_functor) {
  if (is_functor) {
    han |= 1 << handler_bits_t::Functor;
  } else {
    han &= ~(1 << handler_bits_t::Functor);
  }
}

/*static*/ bool
HandlerManager::is_handler_auto(handler_t const& han) {
  return (han >> handler_bits_t::Auto) & 1;
}

/*static*/ bool
HandlerManager::is_handler_functor(handler_t const& han) {
  return (han >> handler_bits_t::Functor) & 1;
}

} // end namespace runtime
