\page node-stats Node Statistics
\brief Manager object profiling data

The node statistics manager component
`vt::vrt::collection::balance::NodeStats`, accessed via `vt::theNodeStats()`
manages instrumentation data from objects in a collection. It holds data per
node on the timing of these objects and communication between them demarcated by
phase and subphase.

When LB is invoked in \vt, the \ref lb-manager passes the node
statistics to the various LB strategies to run the load balancer. The node
statistics component can also dump the statistic data it holds to files, which
can be read externally. The LBAF (Load Balancing Analysis Framework) can also
then read this data to analyze the quality of the load distribution at any phase
in the file.

\section export-lb-stats-file Exporting LB Statistic Files (VOM)

The `NodeStats` component, after collecting statistics from the running program,
can dump these to files in a VOM file (Virtual Object Map). As indicated by the
name, the VOM file specifies the mapping of object to node for each phase along
with statistics for each object (computation time and communication load).

To output VOM files, pass `--vt_lb_stats` to enable output along with
`--vt_lb_stats_dir=<my-directory>` and `--vt_lb_stats_file=<my-base-name>` to
control the directory the files are generated along with the base file
name. With this enabled, \vt will generate a file for each node that contains
the statistics and mapping.

\subsection stats-file-format File Format

The VOM files are output in JSON format, either compressed with brotli
compression (default on) or pure JSON if the argument `--vt_lb_stats_compress`
is set to `false`.

The JSON files contain an array of `phases` that have been captured by \vt and
output to the file. Each phase has an `id` indicating which phase it was while
the application was running. Each phase also has an array of `tasks` that
represent work that was done during that phase. Each task has a `time`,
`resource`, `node`, `entity`, and optionally a list of `subphases`. The `entity`
contains information about the task that performed this work. If that `entity`
is a virtual collection object, it will specify the unique `id` for the object,
and optionally the `index`, `home`, and `collection_id` for that object.

\code{.json}
{
    "phases": [
        {
            "id": 0,
            "tasks": [
                {
                    "entity": {
                        "collection_id": 7,
                        "home": 0,
                        "id": 12884901888,
                        "index": [
                            3
                        ],
                        "type": "object"
                    },
                    "node": 0,
                    "resource": "cpu",
                    "subphases": [
                        {
                            "id": 0,
                            "time": 0.014743804931640625
                        }
                    ],
                    "time": 0.014743804931640625
                },
                {
                    "entity": {
                        "collection_id": 7,
                        "home": 0,
                        "id": 4294967296,
                        "index": [
                            1
                        ],
                        "type": "object"
                    },
                    "node": 0,
                    "resource": "cpu",
                    "subphases": [
                        {
                            "id": 0,
                            "time": 0.013672113418579102
                        }
                    ],
                    "time": 0.013672113418579102
                }
            ]
        },
        {
            "id": 1,
            "tasks": [
                {
                    "entity": {
                        "collection_id": 7,
                        "home": 0,
                        "id": 12884901888,
                        "index": [
                            3
                        ],
                        "type": "object"
                    },
                    "node": 0,
                    "resource": "cpu",
                    "subphases": [
                        {
                            "id": 0,
                            "time": 0.014104127883911133
                        }
                    ],
                    "time": 0.014104127883911133
                }
            ]
        }
    ]
}
\endcode

Each phase in the file may also have a `communications` array that specify any
communication between tasks that occurred during the phase. Each communication
has `type`, which is described below in the following table. Additionally, it
specifies the `bytes`, number of `messages`, and the two entities that were
involved in the operator as `to` and `from`. The entities may be of different
types, like an `object` or `node` depending on the type of communication.

\code{.json}
{
    "phases": [
        {
            "communications": [
                {
                    "bytes": 262.0,
                    "from": {
                        "home": 1,
                        "id": 1,
                        "type": "object"
                    },
                    "messages": 1,
                    "to": {
                        "home": 0,
                        "id": 4294967296,
                        "type": "object"
                    },
                    "type": "SendRecv"
                },
                {
                    "bytes": 96.0,
                    "from": {
                        "home": 0,
                        "id": 4294967296,
                        "type": "object"
                    },
                    "messages": 1,
                    "to": {
                        "id": 1,
                        "type": "node"
                    },
                    "type": "CollectionToNode"
                },
                {
                    "bytes": 259.0,
                    "from": {
                        "id": 0,
                        "type": "node"
                    },
                    "messages": 1,
                    "to": {
                        "home": 0,
                        "id": 0,
                        "type": "object"
                    },
                    "type": "NodeToCollection"
                }
            ],
            "id": 0
        }
    ]
}
\endcode


The type of communication lines up with the enum
`vt::vrt::collection::balance::CommCategory` in the code.

| Value | Enum entry | Description |
| ----- | ---------- | ----------- |
| 1     | `CommCategory::SendRecv` | A send-receive edge between two collection elements |
| 2     | `CommCategory::CollectionToNode` | A send from a collection element to a node |
| 3     | `CommCategory::NodeToCollection` | A send from a node to a collection element |
| 4     | `CommCategory::Broadcast` | A broadcast from a collection element to a whole collection (receive-side) |
| 5     | `CommCategory::CollectionToNodeBcast` | A broadcast from a collection element to all nodes (receive-side) |
| 6     | `CommCategory::NodeToCollectionBcast` | A broadcast from a node to a whole collection (receive-side) |
| 7     | `CommCategory::CollectiveToCollectionBcast` | Collective 'broadcast' from every node to the local collection elements (receive-side) |

For all the broadcast-like edges, the communication logging will occur on the
receive of the broadcast side (one entry per broadcast recipient).
