
#include "handler.h"
#include "bit_common.h"

namespace vt {

/*static*/ HandlerType HandlerManager::make_handler(
  bool const& is_auto, bool const& is_functor, HandlerIdentifierType const& id
) {
  HandlerType new_han = blank_handler;
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

/*static*/ NodeType HandlerManager::get_handler_node(HandlerType const& han) {
  return BitPackerType::get_field<HandlerBitsType::Node, node_num_bits, NodeType>(han);
}

/*static*/ HandlerIdentifierType HandlerManager::get_handler_identifier(
  HandlerType const& han
) {
  return BitPackerType::get_field<
    HandlerBitsType::Identifier, handler_id_num_bits, HandlerIdentifierType
  >(han);
}

/*static*/ void HandlerManager::set_handler_node(
  HandlerType& han, NodeType const& node
) {
  BitPackerType::set_field<HandlerBitsType::Node, node_num_bits>(han, node);
}

/*static*/ void HandlerManager::set_handler_identifier(
  HandlerType& han, HandlerIdentifierType const& id
) {
  BitPackerType::set_field<HandlerBitsType::Identifier, handler_id_num_bits>(
    han, id
  );
}

/*static*/ void HandlerManager::set_handler_auto(
  HandlerType& han, bool const& is_auto
) {
  BitPackerType::bool_set_field<HandlerBitsType::Auto>(han, is_auto);
}

/*static*/ void HandlerManager::set_handler_functor(
  HandlerType& han, bool const& is_functor
) {
  BitPackerType::bool_set_field<HandlerBitsType::Functor>(han, is_functor);
}

/*static*/ bool HandlerManager::is_handler_auto(HandlerType const& han) {
  return BitPackerType::bool_get_field<HandlerBitsType::Auto>(han);
}

/*static*/ bool HandlerManager::is_handler_functor(HandlerType const& han) {
  return BitPackerType::bool_get_field<HandlerBitsType::Functor>(han);
}

} // end namespace vt
