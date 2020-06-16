\page event AsyncEvent
\brief Manage asynchronous events

The async event component `vt::event::AsyncEvent`, accessed via `vt::theEvent()`
manages local and remote events that complete asynchronously. One may create an
event for a `MPI_Request` so the scheduler tests the event as the scheduler
polls. Once may also create other general events that have a unique ID that can
be tested remotely. Parent events group sets of other events (parent, normal, or
MPI events) together to test them for completion in a single operation. The
event manager is mostly designed for internal \vt usage.
