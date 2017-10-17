
#if !defined INCLUDED_MESSAGING_ENVELOPE_H
#define INCLUDED_MESSAGING_ENVELOPE_H

#include "config.h"

#if backend_check_enabled(trace_enabled)
# include "trace_common.h"
#endif

#include <cassert>

namespace vt {

/*
 *  Envelope Type Bits:
 *    001 -> Get Message
 *    010 -> Put Message
 *    100 -> Term Message
 *    ...
 */

enum eEnvelopeType {
  EnvGet = 0,
  EnvPut = 1,
  EnvTerm = 2,
  EnvBroadcast = 3,
  EnvEpochType = 4,
  EnvTagType = 5,
  EnvCallback = 6
};

static constexpr BitCountType const envelope_num_bits = 7;

struct Envelope {
  using isByteCopyable = std::true_type;

  EnvelopeDataType type : envelope_num_bits;
  NodeType dest : node_num_bits;
  HandlerType han : handler_num_bits;
  RefType ref : ref_num_bits;

  #if backend_check_enabled(trace_enabled)
  trace::TraceEventIDType trace_event : trace::trace_event_num_bits;
  #endif
};

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline void envelopeSetTraceEvent(Env& env, trace::TraceEventIDType const& evt) {
  reinterpret_cast<Envelope*>(&env)->trace_event = evt;
}

template <typename Env>
inline trace::TraceEventIDType envelopeGetTraceEvent(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->trace_event;
}
#endif

// Set the type of Envelope

template <typename Env>
inline void setNormalType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type = 0;
}

template <typename Env>
inline void setGetType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvGet;
}

template <typename Env>
inline void setPutType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvPut;
}

template <typename Env>
inline void setTermType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvTerm;
}

template <typename Env>
inline void setBroadcastType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvBroadcast;
}

template <typename Env>
inline void setEpochType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvEpochType;
}

template <typename Env>
inline void setTagType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvTagType;
}

template <typename Env>
inline void setCallbackType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvCallback;
}

// Test the type of Envelope

template <typename Env>
inline bool envelopeIsTerm(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvTerm);
}

template <typename Env>
inline bool envelopeIsBcast(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvBroadcast);
}

template <typename Env>
inline bool envelopeIsEpochType(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvEpochType);
}

template <typename Env>
inline bool envelopeIsTagType(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvTagType);
}

template <typename Env>
inline bool envelopeIsCallbackType(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvCallback);
}

// Get fields of Envelope

template <typename Env>
inline HandlerType envelopeGetHandler(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->han;
}

template <typename Env>
inline NodeType envelopeGetDest(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->dest;
}

// Set fields of Envelope

template <typename Env>
inline void envelopeSetHandler(Env& env, HandlerType const& handler) {
  reinterpret_cast<Envelope*>(&env)->han = handler;
}

template <typename Env>
inline void envelopeSetDest(Env& env, NodeType const& dest) {
  reinterpret_cast<Envelope*>(&env)->dest = dest;
}

// Envelope reference counting functions for memory management

template <typename Env>
inline void envelopeSetRef(Env& env, RefType const& ref = 0) {
  reinterpret_cast<Envelope*>(&env)->ref = ref;
}

template <typename Env>
inline RefType envelopeGetRef(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->ref;
}

template <typename Env>
inline void envelopeRef(Env& env) {
  reinterpret_cast<Envelope*>(&env)->ref++;
}

template <typename Env>
inline void envelopeDeref(Env& env) {
  reinterpret_cast<Envelope*>(&env)->ref--;
}

// Envelope setup functions

template <typename Env>
inline void envelopeSetup(Env& env, NodeType const& dest, HandlerType const& handler) {
  envelopeSetDest(env, dest);
  envelopeSetHandler(env, handler);
}

template <typename Env>
inline void envelopeInit(Env& env) {
  setNormalType(env);
  envelopeSetDest(env, uninitialized_destination);
  envelopeSetHandler(env, uninitialized_handler);
  envelopeSetRef(env, not_shared_message);
}

inline void envelopeInitEmpty(Envelope& env) {
  envelopeInit(env);
}

struct EpochEnvelope {
  using isByteCopyable = std::true_type;

  Envelope env;
  EpochType epoch : epoch_num_bits;
};

struct TagEnvelope {
  using isByteCopyable = std::true_type;

  Envelope env;
  TagType tag : tag_num_bits;
};

struct EpochTagEnvelope {
  using isByteCopyable = std::true_type;

  Envelope env;
  EpochType epoch : epoch_num_bits;
  TagType tag : tag_num_bits;
};

template <typename Env>
inline EpochType envelopeGetEpoch(Env const& env) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->epoch;
  } else if (envelopeIsEpochType(env)) {
    return reinterpret_cast<EpochEnvelope const*>(&env)->epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
    return no_epoch;
  }
}

template <typename Env>
inline void envelopeSetEpoch(Env& env, EpochType const& epoch) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->epoch = epoch;
  } else if (envelopeIsEpochType(env)) {
    reinterpret_cast<EpochEnvelope*>(&env)->epoch = epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
  }
}

template <typename Env>
inline TagType envelopeGetTag(Env const& env) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->tag;
  } else if (envelopeIsTagType(env)) {
    return reinterpret_cast<TagEnvelope const*>(&env)->tag;
  } else {
    assert(0 and "Envelope must be able to hold an tag");
    return no_tag;
  }
}

template <typename Env>
inline void envelopeSetTag(Env& env, TagType const& tag) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->tag = tag;
  } else if (envelopeIsTagType(env)) {
    reinterpret_cast<TagEnvelope*>(&env)->tag = tag;
  } else {
    assert(0 and "Envelope must be able to hold an tag");
  }
}

inline void envelopeInitEmpty(EpochEnvelope& env) {
  envelopeInit(env);
  setEpochType(env);
  envelopeSetEpoch(env, no_epoch);
}

inline void envelopeInitEmpty(TagEnvelope& env) {
  envelopeInit(env);
  setTagType(env);
  envelopeSetTag(env, no_tag);
}

inline void envelopeInitEmpty(EpochTagEnvelope& env) {
  envelopeInit(env);
  setEpochType(env);
  envelopeSetEpoch(env, no_epoch);
  setTagType(env);
  envelopeSetTag(env, no_tag);
}

static_assert(std::is_pod<Envelope>(), "Envelope must be POD");
static_assert(std::is_pod<EpochEnvelope>(), "EpochEnvelope must be POD");
static_assert(std::is_pod<TagEnvelope>(), "TagEnvelope must be POD");
static_assert(std::is_pod<EpochTagEnvelope>(), "EpochTagEnvelope must be POD");

} //end namespace vt

#endif /*INCLUDED_MESSAGING_ENVELOPE_H*/
