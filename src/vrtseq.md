\page vrtseq VirtualSequencer
\brief Sequence task actions @m_span{m-text m-warning} (experimental) @m_endspan

The sequencer component `vt::seq::TaggedSequencerVrt`, accessed via
`vt::theVirtualSeq()` orders operations on a virtual context. If multiple
handlers arrive, one can specify if they are allowed to run in parallel and in
what correct orders that may execute.
