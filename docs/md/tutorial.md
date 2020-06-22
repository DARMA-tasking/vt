\page tutorial Tutorial
\brief Tutorial of how to use \vt

\section tutorial-init-finalize-vt Initializing/Finalizing

To initialize \vt all you must do is invoke the `vt::initialize` call with the
program arguments, which creates and initializes a new \vt runtime. The
initialize call reads the \vt arguments and removes them, leaving the remaining
for the program.

\code{.cpp}
int main(int argc, char** argv) {
  vt::initialize(argc, argv);
   // program here
  vt::finalize();
}
\endcode

If you are already using MPI in your program already, you don't want to call
initialize like this because \vt will initialize MPI itself. Instead, you should
pass a pointer to the live MPI communicator to \vt for it to clone, as so:

\code{.cpp}
int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  vt::initialize(argc, argv, &MPI_COMM_WORLD);
   // program here
  vt::finalize();
  MPI_Finalize();
}
\endcode

\section tutorial-walkthrough Tutorial Code Snippets

This page walks through the tutorial that exists in the source code. See
`vt/tutorial/*.h` for the tutorial pieces in the repository. The main tutorial
example code that stitches the pieces together is located in example:
`vt/tutorial/tutorial_main.h`. The actual code that compiles into an example is
in `vt/examples/tutorial.cc`.

  - Tutorial 1---Basic Concepts
    - 1a: \subpage tutorial-1a
    - 1b: \subpage tutorial-1b
    - 1c: \subpage tutorial-1c
    - 1d: \subpage tutorial-1d
    - 1e: \subpage tutorial-1e
    - 1f: \subpage tutorial-1f
    - 1g: \subpage tutorial-1g
    - 1h: \subpage tutorial-1h
  - Tutorial 2---Collections
    - 2a: \subpage tutorial-2a
    - 2b: \subpage tutorial-2b
  - Tutorial 3---Epochs
    - 3a: \subpage tutorial-3a
