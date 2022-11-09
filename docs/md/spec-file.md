\page spec-file Spec File
\brief Specification File for vt components

VT is using Specification Files for various components (trace and LB). Such files can be used to control when (on which Phases) these components should be enabled/disabled.

The parser will read the following format:

\code
[%] <phase> <range negative> <range positive>
\endcode

The following is an example of a specification file:

\code
0 0 10
%100 -3 3
200 -5 5
\endcode

This specifies that component will be enabled on the following phases:

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
Whether component is enabled is calculated as an OR across all specification
entries. Thus, if a given phase is contained in any spec line, it is
enabled. Note that `0 % 100 = 0`. Therefore, if the above example did not
contain the first line, component would be enabled as:

\code
{
  [0,3], # any phase mod 100 from -3,+3
  [97,103],
  [195,205],
  [297,303], ...
}
\endcode
