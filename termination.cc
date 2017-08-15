#include "transport.h"

namespace runtime { namespace term {

/*static*/ void
TerminationDetector::register_termination_handlers() {

  the_term->new_epoch_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EpochMsg& msg = *static_cast<EpochMsg*>(in_msg);
      the_term->propagate_new_epoch(msg.new_epoch);
    });

  the_term->ready_epoch_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EpochMsg& msg = *static_cast<EpochMsg*>(in_msg);
      the_term->ready_new_epoch(msg.new_epoch);
    });

  the_term->propagate_epoch_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EpochPropagateMsg& msg = *static_cast<EpochPropagateMsg*>(in_msg);
      the_term->propagate_epoch_external(msg.epoch, msg.prod, msg.cons);
    });

  the_term->epoch_finished_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EpochMsg& msg = *static_cast<EpochMsg*>(in_msg);
      the_term->epoch_finished(msg.new_epoch);
    });

  the_term->epoch_continue_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EpochMsg& msg = *static_cast<EpochMsg*>(in_msg);
      the_term->epoch_continue(msg.new_epoch);
    });
}

/*static*/ void
TerminationDetector::register_default_termination_action() {
  the_term->attach_global_term_action([]{
    printf("TD: terminating program\n");
    exit(0);
  });
}

}} //end namespace runtime::term
