\page mem-usage Memory usage tracker
\brief Track memory usage

The memory usage component `vt::util::memory::MemoryUsage`, accessed via
`vt::theMemUsage()` is an optional VT component that tracks memory usage over
time. It can be used with \subpage trace component to write memory usage to
Projections log files to track usage after each registered function executes. It
can be configured to report usage after each LB phase is reached. This component
is backed by a wide range of different reporters---everything from trapping
memory allocation or counting pages allocation by the system.
