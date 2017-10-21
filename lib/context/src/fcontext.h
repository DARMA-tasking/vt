
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
