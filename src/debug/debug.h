
#if ! defined __RUNTIME_TRANSPORT_DEBUG__
#define __RUNTIME_TRANSPORT_DEBUG__

#if DEBUG_ON
  #define debug_print(fmt, arg...)                                 \
    do {                                                           \
      printf("%d: " fmt, the_context->get_node(), ##arg);          \
    } while (0);
#else
  #define debug_print(fmt, arg...)
#endif

#define debug_print_active debug_print
#define debug_print_term debug_print
#define debug_print_event debug_print
#define debug_print_pool debug_print

#endif /*__RUNTIME_TRANSPORT_DEBUG__*/
