\page proc-stats Processor Statistics
\brief Manager object profiling data

The processor statistics manager component
`vt::vrt::collection::balance::ProcStats`, accessed via `vt::theProcStats()`
manages instrumentation data from objects in a collection. It holds data per
node on the timing of these objects and communication between them demarcated by
phase and subphase.
