\page node-lb-data Node LB Data
\brief Manager object profiling data

The node LB data manager component
`vt::vrt::collection::balance::NodeLBData`, accessed via `vt::theNodeLBData()`
manages instrumentation data from objects in a collection. It holds data per
node on the timing of these objects and communication between them demarcated by
phase and subphase.

When LB is invoked in \vt, the \ref lb-manager passes the node
LB data to the various LB strategies to run the load balancer. The node
LB data component can also dump the LB data it holds to files, which
can be read externally. The LBAF (Load Balancing Analysis Framework) can also
then read this data to analyze the quality of the load distribution at any phase
in the file.

\section export-lb-data-file Exporting LB Data Files (VOM)

The `NodeLBData` component, after collecting LB data from the running program,
can dump these to files in a VOM file (Virtual Object Map). As indicated by the
name, the VOM file specifies the mapping of object to node for each phase along
with LB data for each object (computation time and communication load).

To output VOM files, pass `--vt_lb_data` to enable output along with
`--vt_lb_data_dir=<my-directory>` and `--vt_lb_data_file=<my-base-name>` to
control the directory the files are generated along with the base file
name. With this enabled, \vt will generate a file for each node that contains
the LB data and mapping.

\subsection lb-data-file-format File Format

The VOM files are output in JSON format, either compressed with brotli
compression (default on) or pure JSON if the argument `--vt_lb_data_compress`
is set to `false`.

The JSON files contain an array of `phases` that have been captured by \vt and
output to the file. Each phase has an `id` indicating which phase it was while
the application was running. Each phase also has an array of `tasks` that
represent work that was done during that phase. Each task has a `time`,
`resource`, `node`, `entity`, and optionally a list of `subphases`. The `entity`
contains information about the task that performed this work. If that `entity`
is a virtual collection object, it will specify the unique `id` for the object,
and optionally the `index`, `home`, and `collection_id` for that object.

\include examples/LBDatafile_example.json

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
                    "bytes": 1456.0,
                    "from": {
                        "home": 0,
                        "id": 1,
                        "migratable": false,
                        "type": "object"
                    },
                    "messages": 26,
                    "to": {
                        "home": 1,
                        "id": 5,
                        "migratable": false,
                        "type": "object"
                    },
                    "type": "SendRecv"
                },
                {
                    "bytes": 1456.0,
                    "from": {
                        "home": 0,
                        "id": 1,
                        "migratable": false,
                        "type": "object"
                    },
                    "messages": 26,
                    "to": {
                        "home": 2,
                        "id": 9,
                        "migratable": false,
                        "type": "object"
                    },
                    "type": "SendRecv"
                }
            ]
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

\section lb-data-file-validator LB Data File Validator

All input JSON files will be validated using the `JSON_data_files_validator.py` found in the `scripts` directory, which ensures that a given JSON adheres to the following schema:

\include scripts/LBDatafile_schema.py

\section lb-spec-file LB Specification File

In order to customize when LB output is enabled and disabled, a LB
specification file can be passed to \vt via a command-line flag:
`--vt_lb_spec --vt_lb_spec_file=filename.spec`.

For details about vt's Specification File see \ref spec-file