\page phase Phase Manager
\brief Manage phases of time

The phase manager component `vt::phase::PhaseManager`, accessed via
`vt::thePhase()` allows the delineation of collective intervals of time across
all nodes. Load balancing, as well as other components, use phases as a boundary
to perform many operations over an application's execution such as work
redistribution, outputting of statistical data, or flushing trace data.

The main user interface is a call to `thePhase()->nextPhaseCollective()` which
starts the next phase after performing a reduction. Thus, any work that belongs
in the preceding phase should be synchronized by the user before this is called
(e.g., by calling `vt::runInEpochCollective`).

System components along with applications can register hooks with the phase
manager to determine when a new phase is starting, ending, and after migrations
have occurred.
