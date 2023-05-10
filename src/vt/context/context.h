/*
//@HEADER
// *****************************************************************************
//
//                                  context.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONTEXT_CONTEXT_H
#define INCLUDED_VT_CONTEXT_CONTEXT_H

#include <memory>
#include <mpi.h>

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/context/context_attorney_fwd.h"

#if vt_check_enabled(trace_only)
namespace vt { namespace runnable {
struct RunnableNew;
}} /* end namespace vt::runnable */
#else
# include "vt/runnable/runnable.fwd.h"
#endif

#if vt_check_enabled(trace_enabled)
# include "vt/trace/trace_common.h"
#endif

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
   * \brief Gets the current node (analogous to MPI's rank) currently being
   * used.
   *
   * \see \c vt::NodeType
   *
   * \return the node currently being run on
   */
  inline NodeType getNode() const { return thisNode_; }

  /**
   * \brief Get the number of nodes (analogous to MPI's num ranks) being used
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

  /// Used to manage protected access for other VT runtime components
  friend struct ContextAttorney;

  std::string name() override { return "Context"; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | thisNode_
      | numNodes_
      | communicator_;
  }

  /**
   * \brief Get the current running task
   *
   * \return the current running task
   */
  runnable::RunnableNew* getTask() const { return cur_task_; }

  /**
   * \brief Get the node that caused the current running task to execute; i.e.,
   * the node that sent the message to trigger the current runnable.
   *
   * \note If a task is not currently running, this will return the this node
   * ---equivalent to \c theContext()->getNode()
   *
   * For the current task that is executing, get the node that sent the message
   * that caused this runnable to execute. Note, for collection handlers this
   * will not be the logical node that sent the message. It will be the node
   * that last forwarded the message during location discovery.
   *
   * \return the node that sent the message that triggered the current task
   */
  NodeType getFromNodeCurrentTask() const;

#if vt_check_enabled(trace_enabled)
  /**
   * \brief Get the trace event from the current task
   *
   * \note If a task is not currently running, this will return \c no_trace_event
   *
   * \return the trace event on the message that triggered the current task
   */
  trace::TraceEventIDType getTraceEventCurrentTask() const;
#endif

protected:
  /**
   * \brief Set the current running task
   *
   * \param[in] in_task the current running task
   */
  void setTask(runnable::RunnableNew* in_task);

private:
  NodeType thisNode_ = uninitialized_destination;
  NodeType numNodes_ = uninitialized_destination;
  MPI_Comm communicator_ = MPI_COMM_NULL;
  runnable::RunnableNew* cur_task_ = nullptr;
};

}} // end namespace vt::ctx

namespace vt {

extern ctx::Context* theContext();

} // end namespace vt

#endif /*INCLUDED_VT_CONTEXT_CONTEXT_H*/

