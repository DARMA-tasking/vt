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
    {"branch_misses", std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES)}
};
