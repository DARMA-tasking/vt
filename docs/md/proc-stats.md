\page proc-stats Processor Statistics
\brief Manager object profiling data

The processor statistics manager component
`vt::vrt::collection::balance::ProcStats`, accessed via `vt::theProcStats()`
manages instrumentation data from objects in a collection. It holds data per
node on the timing of these objects and communication between them demarcated by
phase and subphase.

When LB is invoked in \vt, the \ref lb-manager passes the processor
statistics to the various LB strategies to run the load balancer. The processor
statistics component can also dump the statistic data it holds to files, which
can be read externally. The LBAF (Load Balancing Analysis Framework) can also
then read this data to analyze the quality of the load distribution at any phase
in the file.
