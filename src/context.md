
\page context Context
\brief Node-aware context

The context component, accessed via `vt::theContext()`, provides context-aware
querying of the current node (analogous to MPI's \c MPI_Comm_rank), number of
nodes (analogous to MPI's \c MPI_Comm_size), and kernel threading/ULT
information if worker threads are enabled. The context also provides the MPI
communicator that an instance of \vt is currently using.

\copybrief vt::ctx::Context
\copydetails vt::ctx::Context

\subsection get-node Current node/rank

\copybrief vt::ctx::Context::getNode()

To get the current node, one may query this method:

\code{.cpp}
vt::NodeType this_node = vt::theContext()->getNode();
\endcode

\subsection get-num-nodes Number of nodes/ranks

\copybrief vt::ctx::Context::getNumNodes()

To get the number of nodes or ranks that an instance of \vt is using, one may
query this method:

\code{.cpp}
vt::NodeType num_nodes = vt::theContext()->getNumNodes();
\endcode

\note The result from \c getNode or \c getNumNodes will depend on the
communicator that was passed to VT during initialization.