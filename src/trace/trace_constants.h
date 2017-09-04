
#if ! defined __RUNTIME_TRANSPORT_TRACE_CONSTANTS__
#define __RUNTIME_TRANSPORT_TRACE_CONSTANTS__

namespace runtime { namespace trace {

enum class TraceConstants : int32_t {
  InvalidTraceType = -1,
  Creation = 1,
  BeginProcessing = 2,
  EndProcessing = 3,
  Enqueue = 4,
  Dequeue = 5,
  BeginComputation = 6,
  EndComputation = 7,
  BeginInterrupt = 8,
  EndInterrupt = 9,
  MessageRecv = 10,
  BeginTrace = 11,
  EndTrace = 12,
  UserEvent = 13,
  BeginIdle = 14,
  EndIdle = 15,
  BeginPack = 16,
  EndPack = 17,
  BeginUnpack = 18,
  EndUnpack = 19,
  CreationBcast = 20,
  CreationMulticast = 21,
  BeginFunc = 22,
  EndFunc = 23,
  MemoryMalloc = 24,
  MemoryFree = 25,
  UserSupplied = 26,
  MemoryUsageCurrent = 27,
  UserSuppliedNote = 28,
  UserSuppliedBracketedNote = 29,
  EndPhase = 30,
  SurrogateBlock = 31,
  UserStat = 32
};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_CONSTANTS__*/
