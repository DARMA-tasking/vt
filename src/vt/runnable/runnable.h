/*
//@HEADER
// *****************************************************************************
//
//                                  runnable.h
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

#if !defined INCLUDED_VT_RUNNABLE_RUNNABLE_H
#define INCLUDED_VT_RUNNABLE_RUNNABLE_H

#include "vt/messaging/message/smart_ptr.h"
#include "vt/context/runnable_context/base.h"

// fwd-declarations for the element types
namespace vt { namespace vrt {

struct VirtualContext;

namespace collection {

struct UntypedCollection;

}}} /* end namespace vt::vrt::collection */

namespace vt { namespace runnable {

/**
 * \struct RunnableNew
 *
 * \brief Holds a runnable active handler along with all the context associated
 * with it to run it independently of the where in the stack it was created.
 */
struct RunnableNew {
  using CtxBasePtr = std::unique_ptr<ctx::Base>;

  template <typename... Args>
  using FnParamType = void(*)(Args...);

  /**
   * \brief Construct a new \c RunnableNew with a message
   *
   * \param[in] in_msg the message to pass to the task
   * \param[in] in_is_threaded whether the handler can be run with a thread
   */
  template <typename U>
  RunnableNew(MsgSharedPtr<U> const& in_msg, bool in_is_threaded)
    : msg_(in_msg.template to<BaseMsgType>()),
      is_threaded_(in_is_threaded)
  { }

  /**
   * \brief Construct a new \c RunnableNew without a message
   *
   * \param[in] in_is_threaded whether the handler can be run with a thread
   */
  explicit RunnableNew(bool in_is_threaded)
    : is_threaded_(in_is_threaded)
  { }

  RunnableNew(RunnableNew&&) = default;
  RunnableNew(RunnableNew const&) = delete;
  RunnableNew& operator=(RunnableNew const&) = delete;
  RunnableNew& operator=(RunnableNew&&) = default;

public:
  /**
   * \brief Add a new context for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename T, typename... Args>
  void addContext(Args&&... args) {
    contexts_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

  /**
   * \brief Set up a handler to run on an collection object
   *
   * \param[in] elm the object pointer
   * \param[in] handler the handler ID bits
   */
  void setupHandlerElement(
    vrt::collection::UntypedCollection* elm, HandlerType handler
  );

  /**
   * \brief Set up a handler to run on an non-collection object
   *
   * \param[in] elm the object pointer
   * \param[in] handler the handler ID bits
   */
  void setupHandlerElement(vrt::VirtualContext* elm, HandlerType handler);

  /**
   * \brief Set up a basic handler to run
   *
   * \param[in] handler the handler ID bits
   * \param[in] is_void whether it's a void handler w/o an associated message
   * \param[in] tag an optional tag
   */
  void setupHandler(
    HandlerType handler, bool is_void = false, TagType tag = no_tag
  );

  /**
   * \brief Run the task!
   *
   * \note If the runnable is non-threaded, \c isDone() will be \c true after
   * this runs. If it is threaded, it might suspend and the runnable might have
   * more work to complete.
   */
  void run();

  /**
   * \brief Get the thread ID associated with the runnable.
   *
   * \note Return \c no_thread_id if the runnable has not started or is not
   * threaded
   *
   * \return the thread ID
   */
  ThreadIDType getThreadID() const { return tid_; }

private:
  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c begin() on them.
   */
  void begin();

  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c end() on them.
   */
  void end();

  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c suspend() on them.
   */
  void suspend();

  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c resume() on them.
   */
  void resume();

public:
  /**
   * \brief Get a context object by the type \c T
   *
   * \note If the type doesn't exist, the function will return \c nullptr
   *
   * \return the pointer to the context object
   */
  template <typename T>
  T* get();

  /**
   * \brief Get the message associated with the runnable
   *
   * \warning Returns a non-owning pointer to message
   *
   * \return the message
   */
  BaseMsgType* getMsg() const { return msg_.get(); }

  /**
   * \brief Check if this runnable is complete or not
   *
   * \note After invoking \c run() it should be complete unless it suspended!
   *
   * \return whether it is done and can be deallocated
   */
  bool isDone() const { return done_; }

private:
  MsgSharedPtr<BaseMsgType> msg_ = nullptr; /**< The associated message */
  bool is_threaded_ = false;                /**< Whether ULTs are supported */
  std::vector<CtxBasePtr> contexts_;        /**< Vector of contexts */
  ActionType task_ = nullptr;               /**< The runnable's task  */
  bool done_ = false;                       /**< Whether task is complete */
  bool suspended_ = false;                  /**< Whether task is suspended */
  ThreadIDType tid_ = no_thread_id;         /**< The thread ID for the task */
};

}} /* end namespace vt::runnable */

#include "vt/runnable/runnable.impl.h"

#endif /*INCLUDED_VT_RUNNABLE_RUNNABLE_H*/
