/*
//@HEADER
// *****************************************************************************
//
//                                  fcontext.h
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

#if !defined INCLUDED_CONTEXT_SRC_FCONTEXT_H
#define INCLUDED_CONTEXT_SRC_FCONTEXT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
  typedef void* fcontext_t;

  struct fcontext_transfer_t {
    fcontext_t ctx;
    void* data;
  };

  struct fcontext_stack_t {
    void* sptr;
    size_t ssize;
  };

  /**
   * Callback definition for context (coroutine)
   */
  using pfn_fcontext = void (*)(fcontext_transfer_t);
  using tfn_fcontext = fcontext_transfer_t(*)(fcontext_transfer_t);
  // typedef void (*pfn_fcontext)(fcontext_transfer_t);
  // typedef fcontext_transfer_t(*tfn_fcontext)(fcontext_transfer_t);

  /**
   * Switches to another context
   * @param to Target context to switch to
   * @param vp Custom user pointer to pass to new context
   */
  fcontext_transfer_t jump_fcontext(fcontext_t const to, void* vp = nullptr);

  /**
   * Make a new context
   * @param sp Pointer to allocated stack memory
   * @param size Stack memory size
   * @param corofn Callback function for context (coroutine)
   */
  fcontext_t make_fcontext(void * sp, size_t size, pfn_fcontext corofn);
  fcontext_t make_fcontext_stack(fcontext_stack_t stack, pfn_fcontext corofn);
  fcontext_transfer_t ontop_fcontext(fcontext_t const to, void * vp, tfn_fcontext fn);

  fcontext_stack_t create_fcontext_stack(size_t size = 0);
  void destroy_fcontext_stack(fcontext_stack_t s);

#ifdef __cplusplus
}
#endif

#endif /*INCLUDED_CONTEXT_SRC_FCONTEXT_H*/
