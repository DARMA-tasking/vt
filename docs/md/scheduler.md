\page scheduler Scheduler
\brief General scheduling of work

The scheduler component `vt::sched::Scheduler`, accessed via `vt::theSched()`
holds pieces of work to execute later that may be prioritized. The scheduler
polls the \vt components to make progress and collect new pieces of work. The
scheduler allows registration of callbacks when the system is idle.

\section calls-to-the-scheduler Calls to the scheduler

To make the scheduler run once, one may invoke the following:

\code{.cpp}
vt::theSched()->scheduler();
\endcode

\copydoc vt::sched::Scheduler::scheduler(bool)

However, if the scheduler needs to be run until a conditions is met, is
recommended that `runSchedulerWhile` be invoked:

\copydoc vt::sched::Scheduler::runSchedulerWhile(std::function<bool()>)
