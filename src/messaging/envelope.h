
#if ! defined __RUNTIME_TRANSPORT_ENVELOPE__
#define __RUNTIME_TRANSPORT_ENVELOPE__

#include "common.h"

#if backend_check_enabled(trace_enabled)
# include "trace_common.h"
#endif

#include <cassert>

namespace runtime {

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
  EnvelopeDataType type : envelope_num_bits;
  NodeType dest : node_num_bits;
  HandlerType han : handler_num_bits;
  ref_t ref : ref_num_bits;

  #if backend_check_enabled(trace_enabled)
  trace::TraceEventIDType trace_event : trace::trace_event_num_bits;
  #endif
};

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline void envelope_set_trace_event(Env& env, trace::TraceEventIDType const& evt) {
  reinterpret_cast<Envelope*>(&env)->trace_event = evt;
}

template <typename Env>
inline trace::TraceEventIDType envelope_get_trace_event(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->trace_event;
}
#endif

// Set the type of Envelope

template <typename Env>
inline void set_normal_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type = 0;
}

template <typename Env>
inline void set_get_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvGet;
}

template <typename Env>
inline void set_put_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvPut;
}

template <typename Env>
inline void set_term_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvTerm;
}

template <typename Env>
inline void set_broadcast_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvBroadcast;
}

template <typename Env>
inline void set_epoch_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvEpochType;
}

template <typename Env>
inline void set_tag_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvTagType;
}

template <typename Env>
inline void set_callback_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvCallback;
}

// Test the type of Envelope

template <typename Env>
inline bool envelope_is_term(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvTerm);
}

template <typename Env>
inline bool envelope_is_bcast(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvBroadcast);
}

template <typename Env>
inline bool envelope_is_epoch_type(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvEpochType);
}

template <typename Env>
inline bool envelope_is_tag_type(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvTagType);
}

template <typename Env>
inline bool envelope_is_callback_type(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvelopeType::EnvCallback);
}

// Get fields of Envelope

template <typename Env>
inline HandlerType envelope_get_handler(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->han;
}

template <typename Env>
inline NodeType envelope_get_dest(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->dest;
}

// Set fields of Envelope

template <typename Env>
inline void envelope_set_handler(Env& env, HandlerType const& handler) {
  reinterpret_cast<Envelope*>(&env)->han = handler;
}

template <typename Env>
inline void envelope_set_dest(Env& env, NodeType const& dest) {
  reinterpret_cast<Envelope*>(&env)->dest = dest;
}

// Envelope reference counting functions for memory management

template <typename Env>
inline void envelope_set_ref(Env& env, ref_t const& ref = 0) {
  reinterpret_cast<Envelope*>(&env)->ref = ref;
}

template <typename Env>
inline ref_t envelope_get_ref(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->ref;
}

template <typename Env>
inline void envelope_ref(Env& env) {
  reinterpret_cast<Envelope*>(&env)->ref++;
}

template <typename Env>
inline void envelope_deref(Env& env) {
  reinterpret_cast<Envelope*>(&env)->ref--;
}

// Envelope setup functions

template <typename Env>
inline void envelope_setup(Env& env, NodeType const& dest, HandlerType const& handler) {
  envelope_set_dest(env, dest);
  envelope_set_handler(env, handler);
}

template <typename Env>
inline void envelope_init(Env& env) {
  set_normal_type(env);
  envelope_set_dest(env, uninitialized_destination);
  envelope_set_handler(env, uninitialized_handler);
  envelope_set_ref(env, not_shared_message);
}

inline void envelope_init_empty(Envelope& env) {
  envelope_init(env);
}

struct EpochEnvelope {
  Envelope env;
  EpochType epoch : epoch_num_bits;
};

struct TagEnvelope {
  Envelope env;
  TagType tag : tag_num_bits;
};

struct EpochTagEnvelope {
  Envelope env;
  EpochType epoch : epoch_num_bits;
  TagType tag : tag_num_bits;
};

template <typename Env>
inline EpochType envelope_get_epoch(Env const& env) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->epoch;
  } else if (envelope_is_epoch_type(env)) {
    return reinterpret_cast<EpochEnvelope const*>(&env)->epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
    return no_epoch;
  }
}

template <typename Env>
inline void envelope_set_epoch(Env& env, EpochType const& epoch) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->epoch = epoch;
  } else if (envelope_is_epoch_type(env)) {
    reinterpret_cast<EpochEnvelope*>(&env)->epoch = epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
  }
}

template <typename Env>
inline TagType envelope_get_tag(Env const& env) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->tag;
  } else if (envelope_is_tag_type(env)) {
    return reinterpret_cast<TagEnvelope const*>(&env)->tag;
  } else {
    assert(0 and "Envelope must be able to hold an tag");
    return no_tag;
  }
}

template <typename Env>
inline void envelope_set_tag(Env& env, TagType const& tag) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->tag = tag;
  } else if (envelope_is_tag_type(env)) {
    reinterpret_cast<TagEnvelope*>(&env)->tag = tag;
  } else {
    assert(0 and "Envelope must be able to hold an tag");
  }
}

inline void envelope_init_empty(EpochEnvelope& env) {
  envelope_init(env);
  set_epoch_type(env);
  envelope_set_epoch(env, no_epoch);
}

inline void envelope_init_empty(TagEnvelope& env) {
  envelope_init(env);
  set_tag_type(env);
  envelope_set_tag(env, no_tag);
}

inline void envelope_init_empty(EpochTagEnvelope& env) {
  envelope_init(env);
  set_epoch_type(env);
  envelope_set_epoch(env, no_epoch);
  set_tag_type(env);
  envelope_set_tag(env, no_tag);
}

static_assert(std::is_pod<Envelope>(), "Envelope must be POD");
static_assert(std::is_pod<EpochEnvelope>(), "EpochEnvelope must be POD");
static_assert(std::is_pod<TagEnvelope>(), "TagEnvelope must be POD");
static_assert(std::is_pod<EpochTagEnvelope>(), "EpochTagEnvelope must be POD");

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_ENVELOPE__*/
