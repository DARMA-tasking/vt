/*
//@HEADER
// *****************************************************************************
//
//                                  perf_data.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_PERF_DATA_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_PERF_DATA_H

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstring>

#include "vt/metrics/perf_event_map.h"

namespace vt { namespace ctx {

/**
 * \struct PerfData
 *
 * \brief Structure for storing Linux perf data structures to track low-level metrics.
 */
struct PerfData
{
public:
    PerfData()
    {
        std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> global_map = vt::thePerfEventMap()->getEventMap();
        if (global_map.empty())
        {
            vtAbort("The PerfEventMap event map is empty; initialize vt::thePerfEventMap->getEventMap(...)");
        }
        else
        {
            const char* env_p = getenv("VT_EVENTS");

            // Check if the environment variable is set
            if (env_p == nullptr) {
                vtWarn("Warning: Environment variable VT_EVENTS not set, defaulting to 'instructions' for the PAPI event set.\n");
                event_names_.push_back("instructions");
            }
            else {
                std::string env_str(env_p);
                std::stringstream ss(env_str);
                std::string item;

                while (std::getline(ss, item, ','))
                {
                    event_names_.push_back(item);
                }
            }

            for (const auto &event_name : event_names_)
            {
                if (global_map.find(event_name) == global_map.end())
                {
                    cleanupBeforeAbort();
                    vtAbort("Event name isn't in known perf events map: " + event_name);
                }

                struct perf_event_attr pe = {};
                pe.type = global_map.at(event_name).first;
                pe.size = sizeof(struct perf_event_attr);
                pe.config = global_map.at(event_name).second;

                pe.disabled = 1;
                pe.exclude_kernel = 1;
                pe.exclude_hv = 1;
                pe.inherit = 1; // Ensure event is inherited by threads

                if (event_name == "instructions") {
                    pe.pinned = 1;
                }

                int fd = perf_event_open(&pe, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
                if (fd == -1)
                {
                    cleanupBeforeAbort();
                    vtAbort("Error opening perf event: " + std::string(strerror(errno)));
                }

                fds.push_back(fd);
            }
        }
    }

    ~PerfData()
    {
        for (int fd : fds)
        {
            if (fd != -1)
            {
                close(fd);
            }
        }
    }

    void start()
    {
        for (int fd : fds)
        {
            if (fd != -1) {
                ioctl(fd, PERF_EVENT_IOC_RESET, 0);
                ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
            }
        }
    }

    void stop()
    {
        for (int fd : fds)
        {
            if (fd != -1) {
                ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
            }
        }
    }

    std::unordered_map<std::string, uint64_t> getMeasurements()
    {
        std::unordered_map<std::string, uint64_t> measurements;
        for (size_t i = 0; i < fds.size(); ++i)
        {
            uint64_t count;
            if (fds[i] != -1 && read(fds[i], &count, sizeof(uint64_t)) != -1) {
                measurements[event_names_[i]] = count;
            }
            else {
                vtWarn("Failed to read perf event data for: " + event_names_[i]);
            }
        }
        return measurements;
    }

private:
    std::vector<int> fds = {};
    std::vector<std::string> event_names_ = {};

    void cleanupBeforeAbort()
    {
        for (int fd : fds)
        {
            if (fd != -1)
            {
                close(fd);
            }
        }
        fds.clear();
    }

    static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags)
    {
        return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    }
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_PERF_DATA_H*/
