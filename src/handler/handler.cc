
#include "handler.h"
#include "utils/bits/bits_common.h"

namespace vt {

/*static*/ HandlerType HandlerManager::makeHandler(
  bool const& is_auto, bool const& is_functor, HandlerIdentifierType const& id
) {
  HandlerType new_han = blank_handler;
  HandlerManager::setHandlerAuto(new_han, is_auto);
  HandlerManager::setHandlerFunctor(new_han, is_functor);
  HandlerManager::setHandlerIdentifier(new_han, id);

  debug_print(
    handler, node,
    "HandlerManager::makeHandler: is_functor=%s, is_auto=%s, id=%d, han=%d\n",
    print_bool(is_functor), print_bool(is_auto), id, new_han
  );

  return new_han;
}

/*static*/ NodeType HandlerManager::getHandlerNode(HandlerType const& han) {
  return BitPackerType::getField<HandlerBitsType::Node, node_num_bits, NodeType>(han);
}

/*static*/ HandlerIdentifierType HandlerManager::getHandlerIdentifier(
  HandlerType const& han
) {
  return BitPackerType::getField<
    HandlerBitsType::Identifier, handler_id_num_bits, HandlerIdentifierType
  >(han);
}

/*static*/ void HandlerManager::setHandlerNode(
  HandlerType& han, NodeType const& node
) {
  BitPackerType::setField<HandlerBitsType::Node, node_num_bits>(han, node);
}

/*static*/ void HandlerManager::setHandlerIdentifier(
  HandlerType& han, HandlerIdentifierType const& id
) {
  BitPackerType::setField<HandlerBitsType::Identifier, handler_id_num_bits>(
    han, id
  );
}

/*static*/ void HandlerManager::setHandlerAuto(
  HandlerType& han, bool const& is_auto
) {
  BitPackerType::boolSetField<HandlerBitsType::Auto>(han, is_auto);
}

/*static*/ void HandlerManager::setHandlerFunctor(
  HandlerType& han, bool const& is_functor
) {
  BitPackerType::boolSetField<HandlerBitsType::Functor>(han, is_functor);
}

/*static*/ bool HandlerManager::isHandlerAuto(HandlerType const& han) {
  return BitPackerType::boolGetField<HandlerBitsType::Auto>(han);
}

/*static*/ bool HandlerManager::isHandlerFunctor(HandlerType const& han) {
  return BitPackerType::boolGetField<HandlerBitsType::Functor>(han);
}

} // end namespace vt
