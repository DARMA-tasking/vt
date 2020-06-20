\page scheduler Scheduler
\brief General scheduling of work

The scheduler component `vt::sched::Scheduler`, accessed via `vt::theSched()`
holds pieces of work to execute later that may be prioritized. The scheduler
polls the \vt components to make progress and collect new pieces of work. The
scheduler allows registration of callbacks when the system is idle.

\section calls-to-the-scheduler Calls to the scheduler

To advance the scheduler, one can call it as follows:

\code{.cpp}
vt::theSched()->scheduler();
\endcode

This polls every component that might generate or complete work, and potentially
runs one piece of available work.

\copydoc vt::sched::Scheduler::scheduler(bool)

However, if the scheduler needs to be run until a condition (or set of
conditions) is met, it is recommended that `runSchedulerWhile` be invoked:

\copydoc vt::sched::Scheduler::runSchedulerWhile(std::function<bool()>)

\section higher-level-calls Higher-level Calls to Wait for Completion

If work is enclosed in an "epoch", the \subpage term can be used to track its
distributed completion. In this case, instead of calling the scheduler directly,
built-in higher-level functions can be used to advance the scheduler until this
work is complete/terminated.

To run the scheduler until an epoch terminates, call the following function:

\code{.cpp}
vt::runSchedulerThrough(my_epoch);
\endcode

Or, to combine the actual enclosed work with the call to wait for its
termination, use the following function:

\code{.cpp}
vt::runInEpochRooted([]{
  // work to do on a single node
});
\endcode

If the work should be executed by all nodes, use a collective epoch::

\code{.cpp}
vt::runInEpochCollective([]{
  // work to do on all nodes
});
\endcode
