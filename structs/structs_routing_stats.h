
#ifndef STRUCTS_ROUTING_STATS_H
#define STRUCTS_ROUTING_STATS_H

#include <vector>
#include <cstddef>
#include <array>

#include "structs_system.h"

/**
****************************************
Structs involving statistics computation
****************************************
**/

struct StatBlock
/*
This struct holds all the mainstream stats that are collected for a tp/tp-batch. It works for latencies,
loads and overall activity loads (from synchronous mode).

In StatBlock, 'cumulatives' = [x_0,x_1,...,x_R] means that there are x_i
 - e.g., paths of hop-length i
 - e.g., processors with a load of i
 - e.g., switches with an overall activity load of i (see 'TimedActivityStatBlock' below).
'average' =
 - e.g., average hop-length
 - e.g., average load over all processors
 - e.g., average overall activity load over all switches (see 'TimedActivityStatBlock' below), etc.
*/
{
    std::vector<std::size_t>    cumulatives{};// [x_0,x_1,...,x_R]
    std::size_t                 number = 0;
                                // Sigma(x_i), e.g., paths with some hop-length, i.e., successful paths
                                // Sigma(x_i), e.g., processors with some load, i.e., healthy processors
                                // Sigma(x_i), e.g., switches with an overall activity load,
                                //  i.e., healthy switches
    std::size_t                 sum = 0;
                                // Sigma(i * x_i), e.g., sum of hop-lengths over all healthy paths
                                // Sigma(i * x_i), e.g., sum of loads over all nodes
                                // Sigma(i * x_i), e.g., sum of overall activity loads over all channels
    double                      average = 0.0;
    double                      std_dev = 0.0;
    double                      coeff_var = 0.0;
    double                      median = 0.0;
    std::size_t                 minimum = 0;
    std::size_t                 maximum = 0;
};

struct TimedActivityStatBlock
/*
After a tp/tp-batch, note that if we are in synchronous mode then as well as (flow-path) load data, every
node/channel stores an activity map M : t -> a_t in which a_t > 0 is the number of flow-paths where the
node/channel has been active in time-slot t, i.e., its t-activity load. So, any flow-path contributes at
most 1 to the eventual value a_t but might contribute to more than one such a_t. The sum
- Sigma[a_t : t in dom(M)] = overall activity load
of some node/channel and is its total number of active time-slots. Note that if the activity vector is
empty then the overall activity load is 0.

When building cumulative vectors, M : t -> a_t results in:
 - ...
 - time-slot t vector [0,n_1,..., n_(a_t), ..........] becoming to [0,n_1,..., n_(a_t) + 1, ..........]
 - ...
so when time-slot t has cumulative vector [0,n_1,...,n_i,...], n_i processors (for example) have a t-activity
load of i. Note that the 0th entry is always 0 as any a_t is always > 0.

This struct holds stats data collected for a tp/tp-batch as regards timed activity data.
We store in 'cumulatives' X, e.g.,
 - X[0] = [0,n_1,...,n_r0] : n_i processors have a 0-activity load of i > 0
 - X[1] = [0,m_1,...,m_r1] : m_j processors have a 1-activity load of j > 0
 - ...
 - X[T] = [0,p_1,...,p_rT] : p_k processors have a T-activity load of k > 0
E.g., 'average' = [x_0,x_1,...,x_T] : the average i-activity load over all processors is x_i.
Note that, e.g., Sigma(n_i) = number of processors that have a non-zero 0-activity load, which is not, in
general, the total number of healthy processors.
*/
{
    std::vector<std::vector<std::size_t>>    cumulatives;
    std::vector<std::size_t>                 number;
    std::vector<std::size_t>                 sum;
    std::vector<double>                      average;
    std::vector<double>                      std_dev;
    std::vector<double>                      coeff_var;
    std::vector<double>                      median;
    std::vector<std::size_t>                 minimum;
    std::vector<std::size_t>                 maximum;
};

struct RoutingStats
/*
This struct holds:
 - a StatBlock for the latency stats
 - an array<StatBlock> for the load stats per processor type + overall
 - an array<StatBlock> for the load stats per channel type + overall
 - an array<StatBlock> for the activity load stats per processor type + overall
 - an array<StatBlock> for the activity load stats per channel type + overall
 - an array<TimedActivityStatBlock> for the timed activity stats per processor type + overall
 - an array<TimedActivityStatBlock> for the timed activity stats per channel type + overall
*/
{
    // latency

    std::size_t                                                          failed_routes = 0;
                                                                         // number of failed routes
    std::size_t                                                          successful_routes = 0;
                                                                         // number of successful routes
    StatBlock                                                            latency{};
                                                                         // latency stats of a tp/tp-batch
    std::size_t                                                          times_tried = 0;
                                                                         // total number of route attempts
    // load (per node/channel-type)

    std::array<StatBlock, System::num_node_types + 1>                    node_load{};
                                                                         // node load stats per type + all
    std::array<StatBlock, System::num_channel_types + 1>                 channel_load{};
                                                                         //  ditto channel
    // activity (per node/channel-type)

    std::array<StatBlock, System::num_node_types + 1>                    node_activity{};
                                                                         // node activity stats per type + all
    std::array<StatBlock, System::num_channel_types + 1>                 channel_activity{};
                                                                         //  ditto channel
    // timed activity (per node/channel-type)

    std::array<TimedActivityStatBlock, System::num_node_types + 1>       node_timed_activity{};
                                                                         // node timed activity loads per type
                                                                         //  + all
    std::array<TimedActivityStatBlock, System::num_channel_types + 1>    channel_timed_activity{};// ditto
};

#endif
