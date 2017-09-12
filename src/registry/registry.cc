
#include "common.h"
#include "registry.h"

namespace vt {

HandlerType Registry::register_new_handler(
  ActiveFunctionType fn, TagType const& tag, bool const& is_collective
) {
  auto const& this_node = the_context->get_node();

  HandlerType new_handle = 0;
  HandlerIdentifierType const& new_identifier =
    is_collective ? cur_ident_collective_++ : cur_ident_++;

  HandlerManagerType::set_handler_node(
    new_handle, is_collective ? uninitialized_destination : this_node
  );
  HandlerManagerType::set_handler_identifier(new_handle, new_identifier);

  if (tag == no_tag) {
    registered_[new_handle] = fn;
  } else {
    tagged_registered_[new_handle][tag] = fn;
  }

  return new_handle;
}

void Registry::swap_handler(
  HandlerType const& han, ActiveFunctionType fn, TagType const& tag
) {
  if (tag == no_tag) {
    auto iter = registered_.find(han);
    assert(
      iter != registered_.end() and "Handler must be registered"
    );
    iter->second = fn;
  } else {
    if (fn == nullptr) {
      auto tag_iter = tagged_registered_[han].find(tag);
      if (tag_iter != tagged_registered_[han].end()) {
        tagged_registered_[han].erase(tag_iter);
        if (tagged_registered_[han].size() == 0) {
          tagged_registered_.erase(tagged_registered_.find(han));
        }
      }
    } else {
      tagged_registered_[han][tag] = fn;
    }
  }
}

void Registry::unregister_handler_fn(
  HandlerType const& han, TagType const& tag
) {
  swap_handler(han, nullptr, tag);
}

HandlerType Registry::register_active_handler(
  ActiveFunctionType fn, TagType const& tag
) {
  return register_new_handler(fn, tag, true);
}

ActiveFunctionType Registry::get_handler_no_tag(HandlerType const& han) {
  auto iter = registered_.find(han);
  if (iter != registered_.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

ActiveFunctionType Registry::get_trigger(HandlerType const& han) {
  auto iter = triggers_.find(han);
  if (iter != triggers_.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

void Registry::save_trigger(HandlerType const& han, ActiveFunctionType fn) {
  printf("save_trigger: han=%d\n", han);
  triggers_[han] = fn;
}

ActiveFunctionType Registry::get_handler(
  HandlerType const& han, TagType const& tag
) {
  if (tag == no_tag) {
    return get_handler_no_tag(han);
  } else {
    auto tag_iter = tagged_registered_.find(han);
    if (tag_iter == tagged_registered_.end()) {
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

} // end namespace vt
