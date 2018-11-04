
#if !defined INCLUDED_REGISTRY_REGISTRY_H
#define INCLUDED_REGISTRY_REGISTRY_H

#include <vector>
#include <unordered_map>
#include <cassert>

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/handler/handler.h"

namespace vt { namespace registry {

struct Registry {
  using HandlerManagerType = HandlerManager;
  using HandlerBitsType = eHandlerBits;
  using TaggerHandlerType = std::tuple<TagType, HandlerType>;
  using ContainerType = std::unordered_map<HandlerType, ActiveClosureFnType>;
  using TagContainerType = std::unordered_map<TagType, ActiveClosureFnType>;
  using HanTagContainerType = std::unordered_map<HandlerType, TagContainerType>;

  Registry() = default;

  HandlerType registerNewHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag,
    bool const& is_collective = false
  );

  void unregisterHandlerFn(
    HandlerType const& han, TagType const& tag = no_tag
  );
  void swapHandler(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  HandlerType registerActiveHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  ActiveClosureFnType getHandler(
    HandlerType const& han, TagType const& tag = no_tag
  );
  ActiveClosureFnType getHandlerNoTag(HandlerType const& han);
  ActiveClosureFnType getTrigger(HandlerType const& han);

  void saveTrigger(HandlerType const& han, ActiveClosureFnType fn);

private:
  ContainerType triggers_;
  ContainerType registered_;
  HanTagContainerType tagged_registered_;
  HandlerIdentifierType cur_ident_collective_ = first_handle_identifier;
  HandlerIdentifierType cur_ident_ = first_handle_identifier;
};

}} //end namespace vt::registry

namespace vt {

extern registry::Registry* theRegistry();

} // end namespace vt

#endif /*INCLUDED_REGISTRY_REGISTRY_H*/
