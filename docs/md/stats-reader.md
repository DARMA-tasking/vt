\page stats-reader LB Restart Reader
\brief Follow input LB distribution

The LB stats restart reader component
`vt::vrt::collection::balance::StatsRestartReader`, accessed via
`vt::theStatsReader()` reads in an input object distribution for a given program
and follows the distribution as specified in the file.

A common flow is the following:
  - Run the program to output stats files (with the flag `--vt_lb_stats`)
  - Input those files to  the LBAF (Load Balancing Analysis Framework) to generate a
new load distribution offline (e.g., to test a new LB strategy).
    - Tell LBAF to generate a new set of stats files that contains a new mapping
      of object to processor
  - Run the program with the `StatsRestartReader` to test this new mapping on
    the actual application
    - Using the options `--vt_lb_stats_dir_in=inputdir --vt_lb_stats_file_in=filename`
