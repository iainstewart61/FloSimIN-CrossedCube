
#ifndef STRUCTS_LATENCY_H
#define STRUCTS_LATENCY_H

#include <vector>
#include <cstddef>

/**
***************
Latency structs
***************
**/

struct Latency
/*
A struct to describe all parameters associated with latency. A global variable 'LAT' will be defined via:
 - Latency LAT;
Any default values are held in 'config_tags'.
*/
{
    bool                        refined_latency;          // if true then 'per_hop_latency' = SUM{others}
    std::vector<std::size_t>    stack_latency;            // stack lat (in msec) of (pr, se, sw, os)
    std::vector<std::size_t>    propagation_latency;      // prop lat (in msec))
    std::vector<std::size_t>    data_transfer_latency;    // data trans lat (in msec)
    std::vector<std::size_t>    average_routing_latency;  // avg. routing lat (in msec)
    std::vector<std::size_t>    per_hop_latency;          // per hop lat (in msec)
    std::size_t                 os_recon_latency;         // optical switch reconfig lat (in msec)
};

#endif
