
#if ! defined __RUNTIME_TRANSPORT_REGISTRY__
#define __RUNTIME_TRANSPORT_REGISTRY__

#include <vector>
#include <unordered_map>
#include <cassert>

#include "config.h"
#include "function.h"
#include "handler.h"

namespace vt {

struct Registry {
  using HandlerManagerType = HandlerManager;
  using HandlerBitsType = eHandlerBits;
  using TaggerHandlerType = std::tuple<TagType, HandlerType>;
  using ContainerType = std::unordered_map<HandlerType, ActiveFunctionType>;
  using TagContainerType = std::unordered_map<TagType, ActiveFunctionType>;
  using HanTagContainerType = std::unordered_map<HandlerType, TagContainerType>;

  Registry() = default;

  HandlerType registerNewHandler(
    ActiveFunctionType fn, TagType const& tag = no_tag,
    bool const& is_collective = false
  );

  void unregisterHandlerFn(
    HandlerType const& han, TagType const& tag = no_tag
  );
  void swapHandler(
    HandlerType const& han, ActiveFunctionType fn, TagType const& tag = no_tag
  );
  HandlerType registerActiveHandler(
    ActiveFunctionType fn, TagType const& tag = no_tag
  );
  ActiveFunctionType getHandler(
    HandlerType const& han, TagType const& tag = no_tag
  );
  ActiveFunctionType getHandlerNoTag(HandlerType const& han);
  ActiveFunctionType getTrigger(HandlerType const& han);

  void saveTrigger(HandlerType const& han, ActiveFunctionType fn);

private:
  ContainerType triggers_;
  ContainerType registered_;
  HanTagContainerType tagged_registered_;
  HandlerIdentifierType cur_ident_collective_ = first_handle_identifier;
  HandlerIdentifierType cur_ident_ = first_handle_identifier;
};

extern std::unique_ptr<Registry> theRegistry;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
