
#if !defined INCLUDED_TERMINATION_TERM_WINDOW_H
#define INCLUDED_TERMINATION_TERM_WINDOW_H

#include "config.h"
#include "epoch/epoch_manip.h"

#include <unordered_set>

namespace vt { namespace term {

struct EpochWindow {

  explicit EpochWindow(bool const in_conform = true)
    : conform_archetype_(in_conform), initialized_(!in_conform)
  { }

private:
  inline bool isArchetypal(EpochType const& epoch);

public:
  void initialize(EpochType const& epoch);

  EpochType getFirst() const { return first_unresolved_epoch_; }
  EpochType getLast()  const { return last_unresolved_epoch_; }

  bool inWindow(EpochType const& epoch) const;
  bool isFinished(EpochType const& epoch) const;
  void addEpoch(EpochType const& epoch);
  void closeEpoch(EpochType const& epoch);

  void clean(EpochType const& epoch);

private:
  // The archetypical epoch for this window container (category,rooted,user,..)
  EpochType archetype_epoch_              = no_epoch;
  // Has this window been initialized with an archetype?
  bool initialized_                       = false;
  // Should the epoch conform to an archetype?
  bool conform_archetype_                 = true;
  // The first unresolved epoch in the window: all epoch <= this are finished
  EpochType first_unresolved_epoch_       = no_epoch;
  // The last unresolved epoch in the current window
  EpochType last_unresolved_epoch_        = no_epoch;
  // The set of epochs finished that are not represented by the window
  std::unordered_set<EpochType> finished_ = {};
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_WINDOW_H*/
