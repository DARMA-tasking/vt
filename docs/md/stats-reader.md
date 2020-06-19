\page stats-reader LB Restart Reader
\brief Follow input LB distribution

The LB stats restart reader component
`vt::vrt::collection::balance::StatsRestartReader`, accessed via
`vt::theStatsReader()` reads in an input object distribution for a given program
and follows the distribution as specified in the file.

A common flow is for the LBAF (Load Balancing Analysis Framework) to generate a
new load distribution offline, producing LB stats file, which are then read by
this component to follow the LB distribution described in those mapping files.
