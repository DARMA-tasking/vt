
#if !defined INCLUDED_TOPOS_LOCATION_RECORD_STATE_STRINGIZE_H
#define INCLUDED_TOPOS_LOCATION_RECORD_STATE_STRINGIZE_H

#include "config.h"
#include "topos/location/record/state.h"

#define print_location_record_state(state) (                      \
    (state) == eLocState::Local ? "eLocState::Local" : (          \
      (state) == eLocState::Remote ? "eLocState::Remote" : (      \
        (state) == eLocState::Invalid ? "eLocState::Invalid" : (  \
          "Unknown eLocState!"                                    \
        )                                                         \
      )                                                           \
    )                                                             \
  )

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_STATE_STRINGIZE_H*/
