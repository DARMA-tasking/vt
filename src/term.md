\page term Termination Detector
\brief Detect termination of work

The termination component `vt::term::TerminationDetector`, accessed via
`vt::theTerm()` detects the transitive completion of work following the causal
chain of messages/events across multiple nodes. It provides global termination
to determine when all work is complete and the schedulers can stop
running. Additionally, it enables the creation of epochs (which stamp message
envelopes) to mark messages as part of a work grouping to detect termination of
all causal events related to a subset of messages in the system.

The termination detector comes with two different detection algorithms: (1)
4-counter wave-based termination for large collective epochs across the whole
system; and, (2) Dijkstra-Scholten parental responsibility termination for
rooted epochs. Epochs are allowed to have other epochs nested within them, thus
forming a graph. The detector tracks the relation between epochs, only making
progress on epochs that do not have a dependency on another epoch terminating
first.

The termination detector also comes with hang detection to detect causes where
no progress can be made due to a user's fault. When a hang is detected, if
configured as so by the user, the detector will dump a DOT graph of the live
epochs and their dependencies.

\section term-collective-example Example of creating a collective epoch

\snippet examples/termination/termination_collective.cc Collective termination example

\section term-rooted-example Example of creating a rooted epoch

\snippet examples/termination/termination_rooted.cc Rooted termination example
