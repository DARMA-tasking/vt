#if !defined INCLUDED_VT_METRICS_EXAMPLE_EVENTS_H
#define INCLUDED_VT_METRICS_EXAMPLE_EVENTS_H

#include <unordered_map>
#include <string>
#include <linux/perf_event.h>

const std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> example_event_map = {
    {"cycles", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES)},
    {"instructions", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS)},
    {"cache_references", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES)},
    {"cache_misses", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES)},
    {"branch_instructions", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS)},
    {"branch_misses", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES)},
    {"fp_arith_inst_retired_scalar_double", std::make_pair(PERF_TYPE_RAW, 0x5301c7)},
    {"fp_arith_inst_retired_scalar_single", std::make_pair(PERF_TYPE_RAW, 0x5302c7)},
    {"fp_arith_inst_retired_128b_packed_double", std::make_pair(PERF_TYPE_RAW, 0x5304c7)},
    {"fp_arith_inst_retired_128b_packed_single", std::make_pair(PERF_TYPE_RAW, 0x5308c7)},
    {"fp_arith_inst_retired_256b_packed_double", std::make_pair(PERF_TYPE_RAW, 0x5310c7)},
    {"fp_arith_inst_retired_256b_packed_single", std::make_pair(PERF_TYPE_RAW, 0x5320c7)},
    {"fp_arith_inst_retired_512b_packed_double", std::make_pair(PERF_TYPE_RAW, 0x5340c7)},
    {"fp_arith_inst_retired_512b_packed_single", std::make_pair(PERF_TYPE_RAW, 0x5380c7)}
};

#endif /*INCLUDED_VT_METRICS_EXAMPLE_EVENTS_H*/
