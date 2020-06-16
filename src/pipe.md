\page pipe PipeManager
\brief Create opaque callback endpoints

The pipe manager component `vt::pipe::PipeManager`, accessed via `vt::theCB()`
allows the creation of general pipes and callbacks between opaque endpoints that
are not revealed through the type. Callbacks allow one to supply a general
endpoint that accepts a type of data without revealing the actual endpoint
instance. For example, one may callback that triggers a handler invocation on a
certain node, broadcasts to a handler, sends to a collection or objgroup,
or broadcasts to a collection or objgroup, etc.

The pipe manager supports more complex use cases of multi-listener endpoints if
one wants to trigger multiple endpoints on potentially different nodes. The
lifetime of a pipe can also be configured---how many invocations are allowed
before the callback is invalid. The pipe manager has a reference count for each
pipe which gets decremented with each signal arrival. By default, callbacks are
infinitely callable and do not expire.

The pipe manager also supports "typed" callbacks where the callee type is
revealed to the caller. Typed callbacks are slightly more efficient because the
type is exposed and registered type-erasure is not required (using lambdas).

\section callback-example Example callbacks

\snippet examples/callback/callback.cc Callback examples
