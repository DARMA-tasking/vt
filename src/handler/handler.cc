
#include "handler.h"
#include "bit_common.h"

namespace runtime {

/*static*/ handler_t
HandlerManager::make_handler(
  bool const& is_auto, bool const& is_functor, handler_identifier_t const& id
) {
  handler_t new_han = blank_handler;
  HandlerManager::set_handler_auto(new_han, is_auto);
  HandlerManager::set_handler_functor(new_han, is_functor);
  HandlerManager::set_handler_identifier(new_han, id);

  debug_print(
    handler, node,
    "HandlerManager::make_handler: is_functor=%s, is_auto=%s, id=%d, han=%d\n",
    print_bool(is_functor), print_bool(is_auto), id, new_han
  );

  return new_han;
}

/*static*/ NodeType
HandlerManager::get_handler_node(handler_t const& han) {
  return bit_packer_t::get_field<handler_bits_t::Node, node_num_bits, NodeType>(han);
}

/*static*/ handler_identifier_t
HandlerManager::get_handler_identifier(handler_t const& han) {
  return bit_packer_t::get_field<
    handler_bits_t::Identifier, handler_id_num_bits, handler_identifier_t
  >(han);
}

/*static*/ void
HandlerManager::set_handler_node(handler_t& han, NodeType const& node) {
  bit_packer_t::set_field<handler_bits_t::Node, node_num_bits>(han, node);
}

/*static*/ void
HandlerManager::set_handler_identifier(
  handler_t& han, handler_identifier_t const& id
) {
  bit_packer_t::set_field<handler_bits_t::Identifier, handler_id_num_bits>(
    han, id
  );
}

/*static*/ void
HandlerManager::set_handler_auto(handler_t& han, bool const& is_auto) {
  bit_packer_t::bool_set_field<handler_bits_t::Auto>(han, is_auto);
}

/*static*/ void
HandlerManager::set_handler_functor(handler_t& han, bool const& is_functor) {
  bit_packer_t::bool_set_field<handler_bits_t::Functor>(han, is_functor);
}

/*static*/ bool
HandlerManager::is_handler_auto(handler_t const& han) {
  return bit_packer_t::bool_get_field<handler_bits_t::Auto>(han);
}

/*static*/ bool
HandlerManager::is_handler_functor(handler_t const& han) {
  return bit_packer_t::bool_get_field<handler_bits_t::Functor>(han);
}

} // end namespace runtime
