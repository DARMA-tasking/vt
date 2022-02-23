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

If, for any reason, you want to predefine configuration, you can do it
by creating `AppConfig` object, setting its members as you wish,
and passing it to `vt::initialize`:

\code{.cpp}
int main(int argc, char** argv) {
  arguments::AppConfig appConfig{};
  appConfig.vt_lb_name = "RotateLB";
  appConfig.vt_lb_stats = true;

  vt::initialize(argc, argv, &appConfig);
   // program here
  vt::finalize();
}
\endcode

You can do also do it if you initialized MPI on your own:

\code{.cpp}
int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  arguments::AppConfig appConfig{};
  appConfig.vt_lb_name = "RotateLB";
  appConfig.vt_lb_stats = true;

  vt::initialize(argc, argv, &MPI_COMM_WORLD, &appConfig);
   // program here
  vt::finalize();
  MPI_Finalize();
}
\endcode

It is worth noting that if you run your application with any of vt's command-line arguments and at the same time you define and pass `AppConfig` to `vt::initialize`, CLI arguments have a higher priority. In other words, if you predefine in source code and give from the command line the same vt's argument, but with a different value, the program will use the CLI one.

There is also an option to use configuration file. Refer to CLI11 documentation for details https://cliutils.github.io/CLI11/book/chapters/config.html. Important thing to remember - CLI11 processes configuration file before command line arguments, so in the end command line arguments might overwrite values defined in configuration file.

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
