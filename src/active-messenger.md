
\page active-messenger Active Messenger
\brief Asynchronous send/receive of messages

The active messenger, accessed via `vt::theMsg()`, asynchronously sends and
receives messages across nodes using MPI internally. When sending a message, it
uses the \vt registry to consistently dispatch messages and data to handlers
(function pointers, functors, or methods) across nodes.

Each message contains an envelope `vt::Envelope` to store meta-data associated
with the message, such as the destination and handler to trigger when it
arrives. Sending a message entails setting up the envelope, optionally
serializing the message (depending on whether the serialize overload is
present), and then using `MPI_Isend` to asynchronously transfer the bytes to the
destination node. On the receive side, the active messenger is always probing
for a incoming message and begins a transfer when it discovers one. The \vt
scheduler polls the active messenger to make progress on any incoming messages.

\copydoc vt::messaging::ActiveMessenger

\section am-simple-example Sending a message

\code{.cpp}
\endcode
