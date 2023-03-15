/*
//@HEADER
// *****************************************************************************
//
//                                  scatter.h
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

#if !defined INCLUDED_VT_COLLECTIVE_SCATTER_SCATTER_H
#define INCLUDED_VT_COLLECTIVE_SCATTER_SCATTER_H

#include "vt/config.h"
#include "vt/collective/scatter/scatter_msg.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/tree/tree.h"
#include "vt/utils/fntraits/fntraits.h"

#include <functional>
#include <cstdlib>

namespace vt { namespace collective { namespace scatter {

/**
 * \struct Scatter
 *
 * \brief Scatter data across all nodes from a single origin
 *
 * Performs an asynchronous scatter over all the nodes in the
 * communicator/runtime.
 */
struct Scatter : virtual collective::tree::Tree {
  using FuncSizeType = std::function<std::size_t(NodeType)>;
  using FuncDataType = std::function<void(NodeType, void*)>;

  /**
   * \internal \brief Construct a scatter manager
   */
  Scatter();

  /**
   * \brief Scatter data to all nodes
   *
   * The functions passed to scatter through the arguments \c size_fn and
   * \c data_fn will not be retained after this call returns.
   *
   * \param[in] total_size total size of data to scatter
   * \param[in] max_proc_size max data to be scattered to any node
   * \param[in] size_fn callback to get size for each node
   * \param[in] data_fn callback to get data for each node
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void scatter(
    std::size_t const& total_size, std::size_t const& max_proc_size,
    FuncSizeType size_fn, FuncDataType data_fn
  );

  /**
   * \brief Scatter data to all nodes
   *
   * The functions passed to scatter through the arguments \c size_fn and
   * \c data_fn will not be retained after this call returns.
   *
   * \param[in] total_size total size of data to scatter
   * \param[in] max_proc_size max data to be scattered to any node
   * \param[in] size_fn callback to get size for each node
   * \param[in] data_fn callback to get data for each node
   */
  template <auto f>
  void scatter(
    std::size_t const& total_size, std::size_t const& max_proc_size,
    FuncSizeType size_fn, FuncDataType data_fn
  ) {
    using MsgT = std::remove_pointer_t<typename FuncTraits<decltype(f)>::Arg1>;
    return scatter<MsgT, f>(total_size, max_proc_size, size_fn, data_fn);
  }

protected:
  /**
   * \internal \brief Receive scattered data down the spanning tree
   *
   * \param[in] msg the scatter message
   */
  void scatterIn(ScatterMsg* msg);

private:
  /**
   * \internal \brief Helper function to scatter data
   *
   * \param[in] node the current code
   * \param[in] ptr pointer to raw data
   * \param[in] elm_size bytes per element of raw data
   * \param[in] size_fn callback to get size for each node
   * \param[in] data_fn callback to get data for each node
   *
   * \return incremented point after scatter is complete
   */
  char* applyScatterRecur(
    NodeType node, char* ptr, std::size_t elm_size, FuncSizeType size_fn,
    FuncDataType data_fn
  );

  /**
   * \internal \brief Active function to receive scattered data
   *
   * \param[in] msg the scatter message
   */
  static void scatterHandler(ScatterMsg* msg);
};

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_VT_COLLECTIVE_SCATTER_SCATTER_H*/
