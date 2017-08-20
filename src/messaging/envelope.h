
#if ! defined __RUNTIME_TRANSPORT_ENVELOPE__
#define __RUNTIME_TRANSPORT_ENVELOPE__

#include "common.h"

namespace runtime {

/*
 *  Envelope Type Bits:
 *    001 -> Get Message
 *    010 -> Put Message
 *    100 -> Term Message
 *    ...
 */

enum EnvelopeType {
  /*Normal = 0*/
  Get = 0,
  Put = 1,
  Term = 2,
  Broadcast = 3,
  EpochType = 4,
  TagType = 5
};

constexpr static int const num_envelope_bits = 6;
constexpr static int const num_handler_bits = 32;
constexpr static int const num_node_bits = 16;
constexpr static int const num_ref_bits = 16;

using envelope_type_t = EnvelopeType;

struct Envelope {
  envelope_datatype_t type : num_envelope_bits;
  node_t dest : num_node_bits;
  handler_t han : num_handler_bits;
  ref_t ref : num_ref_bits;
};

// Set the type of Envelope

template <typename Env>
inline void set_normal_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type = 0;
}

template <typename Env>
inline void set_get_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << EnvelopeType::Get;
}

template <typename Env>
inline void set_put_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << EnvelopeType::Put;
}

template <typename Env>
inline void set_term_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << EnvelopeType::Term;
}

template <typename Env>
inline void set_broadcast_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << EnvelopeType::Broadcast;
}

template <typename Env>
inline void set_epoch_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << EnvelopeType::EpochType;
}

template <typename Env>
inline void set_tag_type(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << EnvelopeType::TagType;
}

// Test the type of Envelope

template <typename Env>
inline bool envelope_is_term(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type & (1 << EnvelopeType::Term);
}

template <typename Env>
inline bool envelope_is_bcast(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type & (1 << EnvelopeType::Broadcast);
}

template <typename Env>
inline bool envelope_is_epoch_type(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type & (1 << EnvelopeType::EpochType);
}

template <typename Env>
inline bool envelope_is_tag_type(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type & (1 << EnvelopeType::TagType);
}

// Get fields of Envelope

template <typename Env>
inline handler_t envelope_get_handler(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->han;
}

template <typename Env>
inline node_t envelope_get_dest(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->dest;
}

// Set fields of Envelope

template <typename Env>
inline void envelope_set_handler(Env& env, handler_t const& handler) {
  reinterpret_cast<Envelope*>(&env)->han = handler;
}

template <typename Env>
inline void envelope_set_dest(Env& env, node_t const& dest) {
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
inline void envelope_setup(Env& env, node_t const& dest, handler_t const& handler) {
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

constexpr static int const num_epoch_bits = 32;
constexpr static int const num_tag_bits = 32;

struct EpochEnvelope {
  Envelope env;
  epoch_t epoch : num_epoch_bits;
};

struct TagEnvelope {
  Envelope env;
  tag_t tag : num_tag_bits;
};

struct EpochTagEnvelope {
  Envelope env;
  epoch_t epoch : num_epoch_bits;
  tag_t tag : num_tag_bits;
};

template <typename Env>
inline epoch_t envelope_get_epoch(Env const& env) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->epoch;
  } else if (envelope_is_epoch_type(env)) {
    return reinterpret_cast<EpochEnvelope const*>(&env)->epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
  }
}

template <typename Env>
inline void envelope_set_epoch(Env& env, epoch_t const& epoch) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->epoch = epoch;
  } else if (envelope_is_epoch_type(env)) {
    reinterpret_cast<EpochEnvelope*>(&env)->epoch = epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
  }
}

template <typename Env>
inline tag_t envelope_get_tag(Env const& env) {
  if (envelope_is_epoch_type(env) and envelope_is_tag_type(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->tag;
  } else if (envelope_is_tag_type(env)) {
    return reinterpret_cast<TagEnvelope const*>(&env)->tag;
  } else {
    assert(0 and "Envelope must be able to hold an tag");
  }
}

template <typename Env>
inline void envelope_set_tag(Env& env, tag_t const& tag) {
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
