\page registry Registry
\brief Registered handlers

The registry component `vt::registry::Registry`, accessed via
`vt::theRegistry()` holds type-safe active handlers for execution across a
distributed machine.

  - The \ref active-messenger uses the registry to store/dispatch active
function and active functor handlers.
  - The \ref objgroup uses the registry to store/dispatch active member
    functions
  - The \ref collection uses the registry to store/dispatch active functions
    with the object pointer and active members.
