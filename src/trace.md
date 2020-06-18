\page trace Tracing
\brief Trace distributed events

The optional trace component `vt::trace::Trace`, accessed via `vt::theTrace()`
builds a distributed trace of events, including VT handlers, user events, and
MPI invocations via the PMPI interface. It outputs
[Projections](http://charm.cs.uiuc.edu/software) log and sts files to enable
performance analysis after execution.

To enable tracing at runtime, the trace component must be enabled at compile
time with cmake. To enable tracing pass the cmake flag:
\code{.cmake}
-Dvt_trace_enabled=1
\endcode
