\page time-trigger Time Triggers
\brief Time-based progress actions

The timed trigger component `vt::timetrigger::TimeTriggerManager`, accessed via
`vt::theTimeTrigger()` manages and coordinates time-based actions that can be
registered by the system and users.

A timed trigger, when registered using the manager, will fire approximately
along that period. For instance, if a trigger is registered with a 100ms period,
it will be called approximately every 100ms while the \vt progress function is
being invoked. If the progress function (or \vt scheduler) is called
infrequently, the triggers may be delayed depending on the period. Also, if
large work units are enqueued in the \vt scheduler that take longer than the
time period, the trigger will fire as often as it can in between these pieces of
work (the component does not use interrupts to trigger actions).

To register a trigger, call
`vt::theTimeTrigger()->addTrigger(100ms, []{ /* my action */});`. The
`addTrigger` method returns a handle to the registered trigger that can be
passed to `vt::theTimeTrigger()->removeTrigger(id);` to stop it from firing at
a certain point.
