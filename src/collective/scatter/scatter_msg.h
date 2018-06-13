
#if !defined INCLUDED_COLLECTIVE_SCATTER_SCATTER_MSG_H
#define INCLUDED_COLLECTIVE_SCATTER_SCATTER_MSG_H

#include "config.h"
#include "messaging/message.h"

namespace vt { namespace collective { namespace scatter {

struct ScatterMsg : ::vt::Message {
  ScatterMsg() = default;

  ScatterMsg(
    std::size_t in_total_bytes, std::size_t in_elm_bytes
  ) : total_bytes_(in_total_bytes), elm_bytes_(in_elm_bytes)
  {}

  std::size_t total_bytes_ = 0;
  std::size_t elm_bytes_ = 0;
  HandlerType user_han = uninitialized_handler;
};

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_COLLECTIVE_SCATTER_SCATTER_MSG_H*/
