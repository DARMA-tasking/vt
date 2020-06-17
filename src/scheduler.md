\page scheduler Scheduler
\brief General scheduling of work

The scheduler component `vt::sched::Scheduler`, accessed via `vt::theSched()`
holds pieces of work to execute later that may be prioritized. The scheduler
polls the \vt components to make progress and collect new pieces of work. The
scheduler allows registration of callbacks when the system is idle.
