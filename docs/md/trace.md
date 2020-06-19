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

The parser will read the  following format:

\code
[%] <phase> <range negative> <range positive>
\endcode

The following is an example of a trace specification:

\code
0 0 10
%100 -3 3
200 -5 5
\endcode

This specifies that tracing will be enabled on the following phases:

\code
{
  [0,10], # phase 0 with offsets 0,+10 (subsumes [0,3] from %100 -3 3)
  [97,103] # any phase % 100 with offset -3,+3
  [195,205] # phase 200 with offsets -5,+5 (subsumes [197,203] from %100 -3 3)
  [297,303] # any phase % 100 with offset -3,+3
  [n%100-3,n%100+3] ... # any phase % 100 with offset -3,+3
}
\endcode

The sets of mod-phase and phase-specific entries must be unique. There may be
overlap across the two sets, but not within them. Having two entries that
start with `%100` or two entries that start with `100` would be invalid and
trigger a parsing error. But having a `%100` and `100` entry is valid.
Whether tracing is enabled is calculated as an OR across all specification
entries. Thus, if a given phase is contained in any spec line, it is
enabled. Note that `0 % 100 = 0`. Therefore, if the above example did not
contain the first line, tracing would be enabled as:

\code
{
  [0,3], # any phase mod 100 from -3,+3
  [97,103],
  [195,205],
  [297,303], ...
}
\endcode
