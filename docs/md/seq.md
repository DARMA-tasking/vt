\page seq Sequencer
\brief Sequence node actions

@m_class{m-label m-danger} **Experimental**

The sequencer component `vt::seq::TaggedSequencer`, accessed via `vt::theSeq()`
orders operations on a node. If multiple handlers arrive, one can specify if
they are allowed to run in parallel and in what correct orders that may execute.
