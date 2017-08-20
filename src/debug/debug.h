
#if ! defined __RUNTIME_TRANSPORT_DEBUG__
#define __RUNTIME_TRANSPORT_DEBUG__

#define print_bool(BOOL) ((BOOL) ? "true" : "false")

#define debug_print_function(fmt, arg...)               \
  do {                                                  \
    printf("%d: " fmt, the_context->get_node(), ##arg); \
  } while (0);

#if DEBUG_ON
  #define debug_print debug_print_function
#else
  #define debug_print(fmt, arg...)
#endif

#define debug_print_active debug_print
#define debug_print_term debug_print
#define debug_print_event debug_print
#define debug_print_pool debug_print
#define debug_print_rdma debug_print

#endif /*__RUNTIME_TRANSPORT_DEBUG__*/
