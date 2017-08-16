
#if ! defined __RUNTIME_TRANSPORT_REGISTRY__
#define __RUNTIME_TRANSPORT_REGISTRY__

#include <vector>
#include <cassert>

#include "common.h"
#include "function.h"

namespace runtime {

struct Registry {
  using container_t = std::vector<active_function_t>;
  using register_count_t = uint32_t;

  Registry() = default;

  handler_t
  register_active_handler(active_function_t fn) {
    registered.resize(cur_empty_slot+1);
    registered.at(cur_empty_slot) = fn;
    cur_empty_slot++;
    return cur_empty_slot-1;
  }

  active_function_t
  get_handler(handler_t const& han) {
    assert(
      registered.size()-1 >= han and "Handler must be registered"
    );
    return registered.at(han);
  }

private:
  container_t registered;

  register_count_t cur_empty_slot = 0;
};

extern std::unique_ptr<Registry> the_registry;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_REGISTRY__*/
