
#if ! defined __RUNTIME_TRANSPORT_REGISTRY__
#define __RUNTIME_TRANSPORT_REGISTRY__

#include <vector>
#include <unordered_map>
#include <cassert>

#include "common.h"
#include "function.h"
#include "handler.h"

namespace runtime {

struct Registry {
  using handler_manager_t = HandlerManager;
  using handler_bits_t = HandlerBits;
  using tagged_HandlerType = std::tuple<TagType, HandlerType>;
  using container_t = std::unordered_map<HandlerType, ActiveFunctionType>;
  using tag_container_t = std::unordered_map<TagType, ActiveFunctionType>;
  using han_tag_container_t = std::unordered_map<HandlerType, tag_container_t>;

  Registry() = default;

  HandlerType
  register_new_handler(
    ActiveFunctionType fn, TagType const& tag = no_tag,
    bool const& is_collective = false
  );

  void
  unregister_handler_fn(HandlerType const& han, TagType const& tag = no_tag);

  void
  swap_handler(
    HandlerType const& han, ActiveFunctionType fn, TagType const& tag = no_tag
  );

  HandlerType
  register_active_handler(ActiveFunctionType fn, TagType const& tag = no_tag);

  ActiveFunctionType
  get_handler(HandlerType const& han, TagType const& tag = no_tag);

  ActiveFunctionType
  get_handler_no_tag(HandlerType const& han);

  ActiveFunctionType
  get_trigger(HandlerType const& han);

  void
  save_trigger(HandlerType const& han, ActiveFunctionType fn);

private:
  container_t triggers;

  container_t registered;

  han_tag_container_t tagged_registered;

  handler_identifier_t cur_ident_collective = first_handle_identifier;

  handler_identifier_t cur_ident = first_handle_identifier;
};

extern std::unique_ptr<Registry> the_registry;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
