/*
//@HEADER
// *****************************************************************************
//
//                                  runnable.h
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

#if !defined INCLUDED_VT_RUNNABLE_RUNNABLE_H
#define INCLUDED_VT_RUNNABLE_RUNNABLE_H

#include "vt/messaging/message/smart_ptr.h"
#include "vt/context/runnable_context/td.h"
#include "vt/context/runnable_context/trace.h"
#include "vt/context/runnable_context/set_context.h"
#include "vt/context/runnable_context/collection.h"
#include "vt/context/runnable_context/lb_data.h"
#include "vt/context/runnable_context/continuation.h"
#include "vt/pool/static_sized/memory_pool_equal.h"
#include "vt/elm/elm_id.h"

// fwd-declarations for the element types
namespace vt { namespace vrt {

struct VirtualContext;

namespace collection {

struct UntypedCollection;

}}} /* end namespace vt::vrt::collection */

namespace vt { namespace runnable {

namespace detail {

struct Contexts {
#if vt_check_enabled(trace_enabled)
  bool has_trace = false;
  ctx::Trace trace;
#endif
  ctx::SetContext setcontext;
  bool has_td = false;
  ctx::TD td;
  bool has_cont = false;
  ctx::Continuation cont;
  bool has_col = false;
  ctx::Collection col;
  bool has_lb = false;
  ctx::LBData lb;
};

} /* end namespace detail */

/**
 * \struct RunnableNew
 *
 * \brief Holds a runnable active handler along with all the context associated
 * with it to run it independently of the where in the stack it was created.
 */
struct RunnableNew {
  template <typename... Args>
  using FnParamType = void(*)(Args...);
  using DispatcherType = auto_registry::BaseHandlersDispatcherPtr::pointer;
  using DispatcherScatterType = auto_registry::BaseScatterDispatcherPtr::pointer;

  /**
   * \brief Construct a new \c RunnableNew with a message
   *
   * \param[in] in_msg the message to pass to the task
   * \param[in] in_is_threaded whether the handler can be run with a thread
   */
  template <typename U>
  RunnableNew(MsgSharedPtr<U> const& in_msg, bool in_is_threaded)
    : msg_(in_msg.template to<BaseMsgType>())
#if vt_check_enabled(fcontext)
    , is_threaded_(in_is_threaded)
#endif
  { }

  /**
   * \brief Construct a new \c RunnableNew without a message
   *
   * \param[in] in_is_threaded whether the handler can be run with a thread
   */
  explicit RunnableNew(bool in_is_threaded)
#if vt_check_enabled(fcontext)
    : is_threaded_(in_is_threaded)
#endif
  { }

  RunnableNew(RunnableNew&&) = default;
  RunnableNew(RunnableNew const&) = delete;
  RunnableNew& operator=(RunnableNew const&) = delete;
  RunnableNew& operator=(RunnableNew&&) = default;

public:
  /**
   * \brief Add a new \c SetContext for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename... Args>
  void addContextSetContext(Args&&... args);

  /**
   * \brief Add a new \c TD for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename... Args>
  void addContextTD(Args&&... args);

  /**
   * \brief Add a new \c Cont for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename... Args>
  void addContextCont(Args&&... args);

  /**
   * \brief Add a new \c Col for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename... Args>
  void addContextCol(Args&&... args);

  /**
   * \brief Add a new \c LB for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename... Args>
  void addContextLB(Args&&... args);

#if vt_check_enabled(trace_enabled)
  /**
   * \brief Add a new \c Trace for this handler
   *
   * \param[in] args arguments to build the context, forwarded to constructor of
   * \c T
   */
  template <typename... Args>
  void addContextTrace(Args&&... args);
#endif

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
   * \brief Set up a handler to run on an object group
   *
   * \param[in] elm the object pointer
   * \param[in] handler the handler ID bits
   */
  void setupHandlerObjGroup(void* obj, HandlerType handler);

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
   */
  void setupHandler(HandlerType handler);

  /**
   * \brief Run the task!
   *
   * \note If the runnable is non-threaded, \c isDone() will be \c true after
   * this runs. If it is threaded, it might suspend and the runnable might have
   * more work to complete.
   */
  void run();

  /**
   * \brief Run the task as a lambda!
   */
  template <typename Callable, typename... Args>
  decltype(auto) runLambda(Callable&& c, Args&&... args) {
    auto start_time = timing::getCurrentTime();
    start(start_time);

    // Arrange a scope guard to call finish() without any sort of dynamic allocation
    struct finisher {
      RunnableNew* r;
      finisher(RunnableNew* in_r) : r(in_r){};
      ~finisher() {
        auto finish_time = timing::getCurrentTime();
        r->finish(finish_time);
      }
    };
    finisher f(this);

    return std::invoke(std::forward<Callable>(c), std::forward<Args>(args)...);
  }

#if vt_check_enabled(fcontext)
  /**
   * \brief Get the thread ID associated with the runnable.
   *
   * \note Return \c no_thread_id if the runnable has not started or is not
   * threaded
   *
   * \return the thread ID
   */
  ThreadIDType getThreadID() const { return tid_; }
#endif

private:
  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c start() on them.
   */
  void start(TimeType time);

  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c finish() on them.
   */
  void finish(TimeType time);

  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c suspend() on them.
   */
  void suspend(TimeType time);

  /**
   * \internal \brief Loop through all the contexts associated with this
   * runnable and invoke \c resume() on them.
   */
  void resume(TimeType time);

public:
  /**
   * \brief Loop through all contexts add run the \c send() method associated
   * with this runnable
   *
   * \param[in] dest the destination element for the send
   * \param[in] bytes the message size
   */
  void send(elm::ElementIDStruct elm, MsgSizeType bytes);

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

#if vt_check_enabled(fcontext)
  /**
   * \brief Check if this runnable is complete or not
   *
   * \note After invoking \c run() it should be complete unless it suspended!
   *
   * \return whether it is done and can be deallocated
   */
  bool isDone() const { return done_; }

  /**
   * \brief Check if this runnable is suspended or not
   *
   * \note If the \c run() method is never called suspended will not be true
   * because it was never started.
   *
   * \return return if it is suspended
   */
  bool isSuspended() const { return suspended_; }
#endif

  /**
   * \internal \brief Operator new for runnables targeting pool
   *
   * \param[in] sz the allocation size
   *
   * \return the new allocation
   */
  static void* operator new(std::size_t sz);

  /**
   * \internal \brief Operator develop for runnables
   *
   * \param[in] ptr the pointer
   */
  static void operator delete(void* ptr);

private:
  detail::Contexts contexts_;               /**< The contexts  */
  MsgSharedPtr<BaseMsgType> msg_ = nullptr; /**< The associated message */
  void* obj_ = nullptr;                     /**< Object pointer */
  union {
    DispatcherType func_;
    DispatcherScatterType func_scat_;
  } f_;
  bool is_scatter_ = false;
#if vt_check_enabled(fcontext)
  bool is_threaded_ = false;                /**< Whether ULTs are supported */
  bool done_ = false;                       /**< Whether task is complete */
  bool suspended_ = false;                  /**< Whether task is suspended */
  ThreadIDType tid_ = no_thread_id;         /**< The thread ID for the task */
#endif
};

struct RunnableNewAlloc {
  static std::unique_ptr<pool::MemoryPoolEqual<sizeof(RunnableNew)>> runnable;
};

}} /* end namespace vt::runnable */

#include "vt/runnable/runnable.impl.h"

#endif /*INCLUDED_VT_RUNNABLE_RUNNABLE_H*/
