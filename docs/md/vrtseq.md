\page vrtseq Virtual Sequencer
\brief Sequence task actions

@m_class{m-label m-danger} **Experimental**

The sequencer component `vt::seq::TaggedSequencerVrt`, accessed via
`vt::theVirtualSeq()` orders operations on a virtual context. If multiple
handlers arrive, one can specify if they are allowed to run in parallel and in
what correct orders that may execute.
