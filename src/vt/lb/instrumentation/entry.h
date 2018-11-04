
#if !defined INCLUDED_LB_INSTRUMENTATION_ENTRY_H
#define INCLUDED_LB_INSTRUMENTATION_ENTRY_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/timing/timing.h"
#include "vt/timing/timing_type.h"

namespace vt { namespace lb { namespace instrumentation {

struct Entry {
  Entry() = default;
  Entry(TimeType const& in_begin, TimeType const& in_end)
    : begin_(in_begin), end_(in_end)
  { }
  Entry(Entry const&) = default;
  Entry(Entry&&) = default;
  Entry& operator=(Entry const&) = default;

  TimeType begin_ = 0.0;
  TimeType end_ = 0.0;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | begin_ | end_;
  }
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_ENTRY_H*/
