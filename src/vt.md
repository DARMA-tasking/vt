
\page introduction Introduction to DARMA/vt
\brief Overview of functionality in \vt

\tableofcontents

\section what-is What is vt?

\vt is an active messaging layer that utilizes C++ object virtualization to
manage virtual endpoints with automatic location management. \vt is directly
built on top of MPI to provide efficient portability across different machine
architectures. Empowered with virtualization, \vt can automatically perform
dynamic load balancing to schedule scientific applications across diverse
platforms with minimal user input.

\vt abstracts the concept of a `node`/`rank`/`worker`/`thread` so a program can
be written in terms of virtual entities that are location independent. Thus,
they can be automatically migrated and thereby executed on varying hardware
resources without explicit programmer mapping, location, and communication
management.

\section vt-features Features in vt

- Active messaging to type-safe handlers across nodes
- Groups for scalable construction of node subsets
- Optional serialization of messages
- Termination detection across entire or subset of DAG with \e epochs
- Opaque callbacks/pipes to generalized endpoints
- Efficient memory pooling for message allocation
- RDMA using MPI one-sided for data transfer
- Asynchronous Collectives across nodes/groups (scatter, async barrier, reduce, ...)
- General scheduler with prioritization
- Built-in interoperability with MPI and threading libraries (Kokkos, OpenMP, ...)
- Object groups for node-level encapsulation
- Virtual contexts for migratable virtualization and dispatch
- Abstractions for multi-dimensional indices, mapping, and linearization
- Virtual collections (dense, sparse, dynamic insertable) for decomposing domains
- Fully distributed load balancer for virtual entities

\section vt-components Components in vt

| Component                   | Singleton           | Details                     |
| --------------------------- | ------------------- | --------------------------- |
| \subpage context            | `vt::theContext()`  | \copybrief context          |
| \subpage active-messenger   | `vt::theMsg()`      | \copybrief active-messenger |

\section vt-hello-world Example

\m_class{m-block m-success}
\parblock
    \m_class{m-code-figure} \parblock
        \code{.cpp}
        struct HelloMsg : vt::Message {
          HelloMsg(vt::NodeType in_from) : from(in_from) { }
          vt::NodeType from = 0;
        };

        void hello_world(HelloMsg* msg) {
          vt::NodeType this_node = vt::theContext()->getNode();
          fmt::print("{}: Hello from node {}\n", this_node, msg->from);
        }

        int main(int argc, char** argv) {
          vt::initialize(argc, arv);

          vt::NodeType this_node = vt::theContext()->getNode();
          vt::NodeType num_nodes = vt::theContext()->getNumNodes();

          if (this_node == 0) {
            auto msg = vt::makeMessage<HelloMsg>(this_node);
            vt::theMsg()->broadcastMsg<HelloMsg, hello_world>(msg.get());
          }

          vt::finalize();
          return 0;
        }
        \endcode

        Running:

        \code{.shell-session}
        $ mpirun -n 4 ./hello_world
        \endcode

        Output:
        \code{.shell-session}
        3: Hello from node 0
        1: Hello from node 0
        2: Hello from node 0
        \endcode

        \note An active message broadcast sends to all nodes except for
the sender (root of the broadcast).
    \endparblock
\endparblock