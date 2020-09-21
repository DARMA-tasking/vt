/*
//@HEADER
// *****************************************************************************
//
//                                  context.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_CONTEXT
#define INCLUDED_CONTEXT

#include <memory>
#include <mpi.h>

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/context/context_attorney_fwd.h"
#include "vt/utils/tls/tls.h"

namespace vt {  namespace ctx {

/** \file */

/**
 * \struct Context context.h vt/context/context.h
 *
 * \brief Used to obtain the current node and other context where a handler
 * executes.
 *
 * Context is a core VT component that provides the ability to pass
 * initialization arguments (through the \c vt::Runtime) and obtain info about
 * the node on which a handler is executing or the number of nodes. It provides
 * functionality analogous to \c MPI_Comm_size and \c MPI_Comm_rank.
 */
struct Context : runtime::component::Component<Context> {
  /**
   * \internal
   * \brief Construct the context.
   *
   * \note MPI must already have been appropriately initialized.
   *
   * \param[in] interop running in interop mode?
   * \param[in] comm the communicator
   */
  Context(bool const interop, MPI_Comm comm);

  ~Context();

  /**
   * \brief Gets the current node (analagous to MPI's rank) currently being
   * used.
   *
   * \see \c vt::NodeType
   *
   * \return the node currently being run on
   */
  inline NodeType getNode() const { return thisNode_; }

  /**
   * \brief Get the number of nodes (analagous to MPI's num ranks) being used
   *
   * \see \c vt::NodeType
   *
   * \return the number of nodes currently being run on
   */
  inline NodeType getNumNodes() const { return numNodes_; }

  /**
   * \brief Get the MPI communicator being used by VT in a given runtime
   * instance
   *
   * \return the \c MPI_Comm being used by VT for communication
   */
  inline MPI_Comm getComm() const { return communicator_; }

  /**
   * \brief Relevant only in threaded mode (e.g., \c std::thread, or OpenMP
   * threads), gets the number of worker threads being used on a given node
   *
   * \see \c vt::WorkerCountType
   *
   * \return the number of worker threads
   */
  inline WorkerCountType getNumWorkers() const { return numWorkers_; }

  /**
   * \brief Informs whether VT is running threaded mode
   *
   * \return whether the VT runtime has workers enabled on a given node
   */
  inline bool hasWorkers() const { return numWorkers_ != no_workers; }

  /**
   * \brief Relevant only in threaded mode (e.g., \c std::thread, or OpenMP
   * threads), gets the current worker thread being used to execute a handler
   *
   * \see \c vt::WorkerIDType
   *
   * \return whether the worker thread ID being used, sequentially numbered
   */
  inline WorkerIDType getWorker() const {
    return AccessClassTLS(Context, thisWorker_);
  }

  /// Used to manage protected access for other VT runtime components
  friend struct ContextAttorney;

  std::string name() override { return "Context"; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | thisNode_
      | numNodes_
      | numWorkers_
      | is_comm_world_;
      // | communicator_; // ompi_communicator_t
  }

protected:
  /// Set the number of workers through the attorney (internal)
  void setNumWorkers(WorkerCountType const worker_count) {
    numWorkers_ = worker_count;
  }
  /// Set the worker through the attorney (internal)
  void setWorker(WorkerIDType const worker) {
    AccessClassTLS(Context, thisWorker_) = worker;
  }

private:
  /// Set the default worker that runs in threaded mode
  void setDefaultWorker();

private:
  NodeType thisNode_ = uninitialized_destination;
  NodeType numNodes_ = uninitialized_destination;
  WorkerCountType numWorkers_ = no_workers;
  MPI_Comm communicator_ = MPI_COMM_WORLD;
  DeclareClassInsideInitTLS(Context, WorkerIDType, thisWorker_, no_worker_id)
};

}} // end namespace vt::ctx

namespace vt {

extern ctx::Context* theContext();

} // end namespace vt

#endif /*INCLUDED_CONTEXT*/

