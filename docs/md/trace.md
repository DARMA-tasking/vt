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

\section tracing-spec-file Tracing Specification File

In order to customize when tracing is enabled and disabled, a trace
specification file can be passed to \vt via a command-line flag:
`--vt_trace_spec --vt_trace_spec_file=filename.spec`.

For details about vt's Specification File see \ref spec-file

\section incremental-tracing Incremental Trace Output

The trace events can be configured to be saved to the file in the incremental matter.
To configure the interval of the flushes use the `--vt_trace_flush_size=X` parameter.
The `X` stands for the number of trace events before the next flush.

\note The incremental flushing will be blocked in the case of an incomplete user notes.
In that scenario there will be no output to the files. All trace events will be kept in memory and will be tried to be flushed on the next interval if the incomplete notes where closed.
