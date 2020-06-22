\mainpage DARMA

*Efficient, distributed task-based programming made easy*

Welcome to the DARMA documentation!

The DARMA project is positioned at the forefront of asynchronous many-task (AMT)
research and development (R&D), providing production-quality dynamic runtime
software for scientific HPC applications. DARMA provides powerful capabilities
for dynamically scheduling tasks, incrementally (re-)balancing loads, data
serialization & movement, and programming abstractions that yield performance
portability. The DARMA team partners with key application/domain-specific teams
to drive development, implement requisite functionality, and production-harden
runtime software tailored to Sandia’s mission-critical applications.

DARMA has two organizations on Github: one for the public facing code and
another for private/internal code, which has not be formally released.

  - DARMA Public Organization: [DARMA-tasking](https://github.com/DARMA-tasking)
  - DARMA Private/Internal Organization: [DARMA-tasking-internal](https://github.com/DARMA-tasking-internal)

The DARMA teams develops an integrated toolkit of codes/repositories that work
together to provide efficient, task-based distributed processing on large
supercomputer architectures. The main public repositories are:

| Module                             | Name                                                           | Links                      |
| ---------------------------------- | -------------------------------------------------------------- | -------------------------- |
| HPC Runtime                        | @m_span{m-text m-success} DARMA/vt @m_endspan (Virtual Transport)                                   | [Github](https://github.com/DARMA-tasking/vt) |
| HPC Serialization                  | @m_span{m-text m-success} DARMA/checkpoint @m_endspan (Checkpointing and Serialization Library)     | [Github](https://github.com/DARMA-tasking/checkpoint) |
| C++ trait detection and checking   | @m_span{m-text m-success} DARMA/detector @m_endspan (C++ trait detector)                            | [Github](https://github.com/DARMA-tasking/detector) |
| HPC LB Simulator                   | @m_span{m-text m-success} DARMA/LBAF @m_endspan (Load Balancing Analysis Framework)                 | [Github](https://github.com/DARMA-tasking/LB-analysis-framework) |
| HPC Serializer compiler analyzer   | @m_span{m-text m-success} DARMA/checkpoint-analyzer @m_endspan (Static verification of serializers) | [Github](https://github.com/DARMA-tasking/checkpoint-member-analyzer) |
| Toolkit documentation              | @m_span{m-text m-success} DARMA/docs @m_endspan                                                     | [Docs](https://github.com/DARMA-tasking/DARMA-tasking.github.io) |

\section intro-darma-vt Learn about DARMA/vt

DARMA/vt is the main runtime in the toolkit that provides tasking
functionality. To learn more about \vt, read the \subpage introduction.
