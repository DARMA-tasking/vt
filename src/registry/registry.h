
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
  using HandlerManagerType = HandlerManager;
  using HandlerBitsType = eHandlerBits;
  using TaggerHandlerType = std::tuple<TagType, HandlerType>;
  using ContainerType = std::unordered_map<HandlerType, ActiveFunctionType>;
  using TagContainerType = std::unordered_map<TagType, ActiveFunctionType>;
  using HanTagContainerType = std::unordered_map<HandlerType, TagContainerType>;

  Registry() = default;

  HandlerType register_new_handler(
    ActiveFunctionType fn, TagType const& tag = no_tag,
    bool const& is_collective = false
  );

  void unregister_handler_fn(
    HandlerType const& han, TagType const& tag = no_tag
  );
  void swap_handler(
    HandlerType const& han, ActiveFunctionType fn, TagType const& tag = no_tag
  );
  HandlerType register_active_handler(
    ActiveFunctionType fn, TagType const& tag = no_tag
  );
  ActiveFunctionType get_handler(
    HandlerType const& han, TagType const& tag = no_tag
  );
  ActiveFunctionType get_handler_no_tag(HandlerType const& han);
  ActiveFunctionType get_trigger(HandlerType const& han);

  void save_trigger(HandlerType const& han, ActiveFunctionType fn);

private:
  ContainerType triggers_;
  ContainerType registered_;
  HanTagContainerType tagged_registered_;
  HandlerIdentifierType cur_ident_collective_ = first_handle_identifier;
  HandlerIdentifierType cur_ident_ = first_handle_identifier;
};

extern std::unique_ptr<Registry> the_registry;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
