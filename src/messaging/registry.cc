
#include "common.h"
#include "registry.h"
#include "transport.h"

namespace runtime {

node_t
Registry::get_handler_node(handler_t const& han) {
  node_t const node = static_cast<node_t>(han >> handler_bits_t::Node);
  return node;
}

handler_identifier_t
Registry::get_handler_identifier(handler_t const& han) {
  handler_identifier_t const id = static_cast<handler_identifier_t>(
    han >> handler_bits_t::Identifier
  );
  return id;
}

void
Registry::set_handler_node(handler_t& han, node_t const& node) {
  han |= node << handler_bits_t::Node;
}

void
Registry::set_handler_identifier(handler_t& han, handler_identifier_t const& ident) {
  han |= ident << handler_bits_t::Identifier;
}

handler_t
Registry::register_new_handler(
  active_function_t fn, bool const& is_collective
) {
  auto const& this_node = the_context->get_node();

  handler_t new_handle = 0;
  handler_identifier_t const& new_identifier =
    is_collective ? cur_ident_collective++ : cur_ident++;

  set_handler_node(new_handle, is_collective ? uninitialized_destination : this_node);
  set_handler_identifier(new_handle, new_identifier);

  registered.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(new_handle),
    std::forward_as_tuple(fn)
  );

  return new_handle;
}

} // end namespace runtime
