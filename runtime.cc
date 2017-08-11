
#include <memory>

#include "transport.h"

namespace runtime {

std::unique_ptr<Registry> the_registry = std::make_unique<Registry>();
std::unique_ptr<ActiveMessenger> the_msg = std::make_unique<ActiveMessenger>();
std::unique_ptr<AsyncEvent> the_event = std::make_unique<AsyncEvent>();
std::unique_ptr<Context> the_context = nullptr;

} //end namespace runtime
