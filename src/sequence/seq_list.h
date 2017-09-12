
#if ! defined __RUNTIME_TRANSPORT_SEQ_LIST__
#define __RUNTIME_TRANSPORT_SEQ_LIST__

#include "common.h"
#include "seq_common.h"

namespace runtime { namespace seq {

struct SeqList {
  using SeqFunType = std::function<bool()>;

  SeqList(SeqType const& this_seq_in)
    : this_seq_(this_seq_in)
  { }

  void add_action(SeqFunType const& fn) {
    lst_.push_back(fn);
  }

  void progress() {
    bool performed_action = true;
    while (ready_ and performed_action) {
      if (lst_.size() > 0) {
        auto x = lst_.front();
        lst_.pop_front();
        bool const should_block = x();
        if (should_block) {
          ready_ = false;
        }
        performed_action = true;
      } else {
        performed_action = false;
      }
    }
  }

  void make_ready() {
    ready_ = true;
  }

private:
  SeqType this_seq_ = no_seq;

  bool ready_ = true;

  std::list<SeqFunType> lst_;
};

}} //end namespace runtime::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_LIST__*/
