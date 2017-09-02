
#include "common.h"
#include "registry.h"

namespace runtime {

handler_t
Registry::register_new_handler(
  active_function_t fn, tag_t const& tag, bool const& is_collective
) {
  auto const& this_node = the_context->get_node();

  handler_t new_handle = 0;
  handler_identifier_t const& new_identifier =
    is_collective ? cur_ident_collective++ : cur_ident++;

  handler_manager_t::set_handler_node(
    new_handle, is_collective ? uninitialized_destination : this_node
  );
  handler_manager_t::set_handler_identifier(new_handle, new_identifier);

  if (tag == no_tag) {
    registered[new_handle] = fn;
  } else {
    tagged_registered[new_handle][tag] = fn;
  }

  return new_handle;
}

void
Registry::swap_handler(
  handler_t const& han, active_function_t fn, tag_t const& tag
) {
  if (tag == no_tag) {
    auto iter = registered.find(han);
    assert(
      iter != registered.end() and "Handler must be registered"
    );
    iter->second = fn;
  } else {
    if (fn == nullptr) {
      auto tag_iter = tagged_registered[han].find(tag);
      if (tag_iter != tagged_registered[han].end()) {
        tagged_registered[han].erase(tag_iter);
        if (tagged_registered[han].size() == 0) {
          tagged_registered.erase(tagged_registered.find(han));
        }
      }
    } else {
      tagged_registered[han][tag] = fn;
    }
  }
}

void
Registry::unregister_handler_fn(handler_t const& han, tag_t const& tag) {
  swap_handler(han, nullptr, tag);
}

handler_t
Registry::register_active_handler(active_function_t fn, tag_t const& tag) {
  return register_new_handler(fn, tag, true);
}

active_function_t
Registry::get_handler_no_tag(handler_t const& han) {
  auto iter = registered.find(han);
  if (iter != registered.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

active_function_t
Registry::get_trigger(handler_t const& han) {
  auto iter = triggers.find(han);
  if (iter != triggers.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

void
Registry::save_trigger(handler_t const& han, active_function_t fn) {
  printf("save_trigger: han=%d\n", han);
  triggers[han] = fn;
}

active_function_t
Registry::get_handler(handler_t const& han, tag_t const& tag) {
  if (tag == no_tag) {
    return get_handler_no_tag(han);
  } else {
    auto tag_iter = tagged_registered.find(han);
    if (tag_iter == tagged_registered.end()) {
      return get_handler_no_tag(han);
    } else {
      auto iter = tag_iter->second.find(tag);
      if (iter != tag_iter->second.end()) {
        return iter->second;
      } else {
        return get_handler_no_tag(han);
      }
    }
  }
}

} // end namespace runtime
