\page introduction Introduction
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
  - Termination detection across all work or subsets of work with \e epochs
  - Opaque callbacks/pipes to generalized endpoints
  - Efficient memory pooling for message allocation
  - RDMA using MPI one-sided for data transfer
  - Asynchronous Collectives across nodes/groups (scatter, async barrier, reduce, ...)
  - General scheduler with prioritization
  - Built-in interoperability with MPI and threading libraries (e.g. Kokkos, fcontext)
  - Object groups for node-level encapsulation
  - Virtual contexts for migratable virtualization and dispatch
  - Abstractions for multi-dimensional indices, mapping, and linearization
  - Virtual collections (dense, sparse, dynamic insertable) for decomposing domains
  - Fully distributed load balancer for virtual entities

\section vt-components Components in vt

| Component                   | Singleton              | Details                     | Type                                           |
| --------------------------- | ---------------------- | --------------------------- | ---------------------------------------------- |
| \subpage context            | `vt::theContext()`     | \copybrief context          | @m_class{m-label m-success} **Core**           |
| \subpage active-messenger   | `vt::theMsg()`         | \copybrief active-messenger | @m_class{m-label m-success} **Core**           |
| \subpage collection         | `vt::theCollection()`  | \copybrief collection       | @m_class{m-label m-success} **Core**           |
| \subpage collective         | `vt::theCollective()`  | \copybrief collective       | @m_class{m-label m-success} **Core**           |
| \subpage event              | `vt::theEvent()`       | \copybrief event            | @m_class{m-label m-success} **Core**           |
| \subpage group              | `vt::theGroup()`       | \copybrief group            | @m_class{m-label m-success} **Core**           |
| \subpage lb-manager         | `vt::theLBManager()`   | \copybrief lb-manager       | @m_class{m-label m-warning} **Optional**       |
| \subpage location           | `vt::theLocMan()`      | \copybrief location         | @m_class{m-label m-success} **Core**           |
| \subpage mem-usage          | `vt::theMemUsage()`    | \copybrief mem-usage        | @m_class{m-label m-warning} **Optional**       |
| \subpage objgroup           | `vt::theObjGroup()`    | \copybrief objgroup         | @m_class{m-label m-success} **Core**           |
| \subpage pipe               | `vt::theCB()`          | \copybrief pipe             | @m_class{m-label m-success} **Core**           |
| \subpage node-lb-data         | `vt::theNodeLBData()`   | \copybrief node-lb-data       | @m_class{m-label m-warning} **Optional**       |
| \subpage phase              | `vt::thePhase()`       | \copybrief phase            | @m_class{m-label m-success} **Core**           |
| \subpage pool               | `vt::thePool()`        | \copybrief pool             | @m_class{m-label m-success} **Core**           |
| \subpage rdma               | `vt::theRDMA()`        | \copybrief rdma             | @m_class{m-label m-danger} **Experimental**    |
| \subpage rdmahandle         | `vt::theHandleRDMA()`  | \copybrief rdmahandle       | @m_class{m-label m-warning} **Optional**       |
| \subpage registry           | `vt::theRegistry()`    | \copybrief registry         | @m_class{m-label m-success} **Core**           |
| \subpage scheduler          | `vt::theSched()`       | \copybrief scheduler        | @m_class{m-label m-success} **Core**           |
| \subpage term               | `vt::theTerm()`        | \copybrief term             | @m_class{m-label m-success} **Core**           |
| \subpage epoch              | `vt::theEpoch()`       | \copybrief epoch            | @m_class{m-label m-success} **Core**           |
| \subpage time-trigger       | `vt::theTimeTrigger()` | \copybrief time-trigger     | @m_class{m-label m-warning} **Optional**       |
| \subpage trace              | `vt::theTrace()`       | \copybrief trace            | @m_class{m-label m-warning} **Optional**       |
| \subpage lb-data-reader       | `vt::theLBDataReader()` | \copybrief lb-data-reader     | @m_class{m-label m-warning} **Optional**       |


\section how-to-build-intro How to build

\vt can be built with cmake or inside a docker container. Learn \ref vt-build

\section vt-hello-world Hello World Example

\m_class{m-block m-success}
\parblock
    \m_class{m-code-figure} \parblock
        \code{.cpp}
        bool done = false;

        struct HelloMsg : vt::Message {
          HelloMsg(vt::NodeType in_from) : from(in_from) { }
          vt::NodeType from = 0;
        };

        void hello_world(HelloMsg* msg) {
          vt::NodeType this_node = vt::theContext()->getNode();
          fmt::print("{}: Hello from node {}\n", this_node, msg->from);
          done = true;
        }

        int main(int argc, char** argv) {
          vt::initialize(argc, arv);

          vt::NodeType this_node = vt::theContext()->getNode();
          vt::NodeType num_nodes = vt::theContext()->getNumNodes();

          if (this_node == 0) {
            auto msg = vt::makeMessage<HelloMsg>(this_node);
            vt::theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
            done = true;
          }

          // Run the scheduler until all nodes are done
          vt::runSchedulerWhile([]{ return !done; });

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

\section License

@m_class{m-note m-dim}

@parblock
Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
(NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
Government retains certain rights in this software.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
@endparblock
