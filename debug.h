
#if ! defined __RUNTIME_TRANSPORT_DEBUG__
#define __RUNTIME_TRANSPORT_DEBUG__

#if DEBUG_ON
#define DEBUG_PRINT(fmt, arg...)                                 \
  do {                                                           \
    printf("%d: " fmt, the_context->get_node(), ##arg);          \
  } while (0);
#else
#define DEBUG_PRINT(label, fmt, arg...)
#endif

#endif /*__RUNTIME_TRANSPORT_DEBUG__*/
