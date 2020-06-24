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

\section export-lb-stats-file Exporting LB Statistic Files (VOM)

The `ProcStats` component, after collecting statistics from the running program,
can dump these to files in a VOM file (Virtual Object Map). As indicated by the
name, the VOM file specifies the mapping of object to node for each phase along
with statistics for each object (computation time and communication load).

To output VOM files, pass `--vt_lb_stats` to enable output along with
`--vt_lb_stats_dir=<my-directory>` and `--vt_lb_stats_file=<my-base-name>` to
control the directory the files are generated along with the base file
name. With this enabled, \vt will generate a file for each node that contains
the statistics and mapping.

\subsection stats-file-format File Format

Each line in the file will one of two formats. The first line is a computation
time line for each phase, that breaks time down into subphases:

\code
<phase>, <object-id>, <time-in-seconds> <#-of-subphases> '[' [<subphase-time-1>] ... [<subphase-time-N>] ']'
\endcode

The second line format is a communication line:

\code
<phase>, <object-id1-to/recv>, <object-id2-from/send>, <num-bytes>, <comm-type={1..6}>
\endcode


Where `<comm-type>` is the type of communication occurred. The type of
communication lines up the enum `vt::vrt::collection::balance::CommCategory` in
the code.

| Value | Enum entry | Description |
| ----- | ---------- | ----------- |
| 1     | `CommCategory::SendRecv` | A send-receive edge between two collection elements |
| 2     | `CommCategory::CollectionToNode` | A send from a collection element to a node |
| 3     | `CommCategory::NodeToCollection` | A send from a node to a collection element |
| 4     | `CommCategory::Broadcast` | A broadcast from a collection element to a whole collection (receive-side) |
| 5     | `CommCategory::CollectionToNodeBcast` | A broadcast from a collection element to all nodes (receive-side) |
| 6     | `CommCategory::NodeToCollectionBcast` | A broadcast from a node to a whole collection (receive-side) |

For all the broadcast-like edges, the communication logging will occur on the
receive of the broadcast side (one entry per broadcast recipient).
