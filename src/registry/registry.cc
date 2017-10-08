
#include "config.h"
#include "registry.h"

namespace vt {

HandlerType Registry::registerNewHandler(
  ActiveClosureFnType fn, TagType const& tag, bool const& is_collective
) {
  auto const& this_node = theContext->getNode();

  HandlerType new_handle = 0;
  HandlerIdentifierType const& new_identifier =
    is_collective ? cur_ident_collective_++ : cur_ident_++;

  HandlerManagerType::setHandlerNode(
    new_handle, is_collective ? uninitialized_destination : this_node
  );
  HandlerManagerType::setHandlerIdentifier(new_handle, new_identifier);

  if (tag == no_tag) {
    registered_[new_handle] = fn;
  } else {
    tagged_registered_[new_handle][tag] = fn;
  }

  return new_handle;
}

void Registry::swapHandler(
  HandlerType const& han, ActiveClosureFnType fn, TagType const& tag
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

void Registry::unregisterHandlerFn(
  HandlerType const& han, TagType const& tag
) {
  swapHandler(han, nullptr, tag);
}

HandlerType Registry::registerActiveHandler(
  ActiveClosureFnType fn, TagType const& tag
) {
  return registerNewHandler(fn, tag, true);
}

ActiveClosureFnType Registry::getHandlerNoTag(HandlerType const& han) {
  auto iter = registered_.find(han);
  if (iter != registered_.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

ActiveClosureFnType Registry::getTrigger(HandlerType const& han) {
  auto iter = triggers_.find(han);
  if (iter != triggers_.end()) {
    return iter->second;
  } else {
    return nullptr;
  }
}

void Registry::saveTrigger(HandlerType const& han, ActiveClosureFnType fn) {
  printf("save_trigger: han=%d\n", han);
  triggers_[han] = fn;
}

ActiveClosureFnType Registry::getHandler(
  HandlerType const& han, TagType const& tag
) {
  if (tag == no_tag) {
    return getHandlerNoTag(han);
  } else {
    auto tag_iter = tagged_registered_.find(han);
    if (tag_iter == tagged_registered_.end()) {
      return getHandlerNoTag(han);
    } else {
      auto iter = tag_iter->second.find(tag);
      if (iter != tag_iter->second.end()) {
        return iter->second;
      } else {
        return getHandlerNoTag(han);
      }
    }
  }
}

} // end namespace vt
