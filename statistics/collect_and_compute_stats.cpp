
#include "collect_and_compute_stats.h"
#include "core_cumulatives.h"
#include "../data_logging/data_logging.h"
#include "../globals.h"
#include "../structs/structs_system.h"

using std::vector;
using std::size_t;

/**
This cpp collects and computes latency, load and activity statistics.

Functions:
Collect and compute latency data:
 - compute_latency_data()
 - collect_batch_latency_data()
Collect load data and compute load stats
 - collect_load_data()
 - compute_load_stats()
 - collect_batch_load_data()
Collect activity data and compute activity stats
 - collect_activity_data()
 - compute_activity_stats()
 - collect_batch_activity_data()
**/


/**
********************************
Collect and compute latency data
********************************
**/

void compute_latency_data(// supplied data:
                          const size_t             failed_routes,
                          const size_t             successful_routes,
                          const vector<size_t>&    cumulative_hop_lengths,
                          const size_t             times_tried,
                          // computed data:
                          RoutingStats&            rs)
/*
After a tp/tp-batch, compute and store all the latency data within a 'RoutingStats' class object.
All stats data is raw in that no decimal precision is employed.
*/
{
    rs.failed_routes = failed_routes;
    rs.successful_routes = successful_routes;
    rs.latency.cumulatives = cumulative_hop_lengths;
    rs.times_tried = times_tried;
    rs.latency.number = compute_cumulative_number(cumulative_hop_lengths);  // = number of healthy routes
    rs.latency.sum = compute_cumulative_sum(cumulative_hop_lengths);
    rs.latency.average = compute_cumulative_average(rs.latency.sum, rs.latency.number);
                                                                            // average over healthy routes
    rs.latency.std_dev = compute_cumulative_std_dev(cumulative_hop_lengths, rs.latency.average,
                                                    rs.latency.number);
    rs.latency.coeff_var = compute_cumulative_coeff_var(rs.latency.average, rs.latency.std_dev);
    rs.latency.median = compute_cumulative_median(cumulative_hop_lengths, rs.latency.number);
    rs.latency.minimum = compute_cumulative_minimum(cumulative_hop_lengths);
    rs.latency.maximum = compute_cumulative_maximum(cumulative_hop_lengths);
}

void collect_batch_latency_data(// supplied data:
                                const size_t                    start_tp,
                                const size_t                    end_tp,
                                const vector<RoutingStats>&     tp_stats,
                                // computed data:
                                size_t&                         batch_failed_routes,
                                size_t&                         batch_successful_routes,
                                vector<size_t>&                 batch_cumulative_hop_lengths,
                                size_t&                         batch_times_tried)
/*
Collect the batch latency data from the tps in 'tp_stats' where the batch goes from tp 'start_tp'
to 'end_tp' - 1.
*/
{
    batch_failed_routes = 0;
    batch_successful_routes = 0;
    batch_cumulative_hop_lengths.clear();
    batch_times_tried = 0;

    for (size_t i = start_tp; i < end_tp; ++i)
    {
        batch_failed_routes += tp_stats[i].failed_routes;
        batch_successful_routes += tp_stats[i].successful_routes;
        augment_cumulative(tp_stats[i].latency.cumulatives, batch_cumulative_hop_lengths);
        batch_times_tried +=  tp_stats[i].times_tried;
    }
}

/**
****************************************
Collect load data and compute load stats
****************************************
**/

void collect_load_data(// supplied data:
                       const vector<node>&    int_net,
                       // computed data:
                       RoutingStats&          rs)
/*
After a tp-run, collect all the load data within a 'routing_stats' class object. Note that we only collect
loads of healthy nodes/channels.
*/
{
    size_t target;
    size_t target_type;
    size_t channel_type;

    for (size_t n_type = 0; n_type < System::num_node_types; ++n_type)// over all node types
    {
        for (size_t source = G.node_types[n_type].first; source < G.node_types[n_type].last; ++source)
        {                                                                     // over all nodes of a type
            if (int_net[source].node_get_health() == System::healthy)// only log healthy node data
            {
                augment_cumulative(int_net[source].node_get_load(),
                                   rs.node_load[n_type].cumulatives);
                size_t num_ports = int_net[source].node_get_num_ports();
                for (size_t port = 0; port < num_ports; ++port)// over all ports
                {
                    if (int_net[source].node_get_port_health(System::out, port) == System::healthy)
                    {                                                    // only log healthy channel data
                        target = int_net[source].node_get_port_node(System::out, port);// target of chan.
                        target_type = int_net[target].node_get_type();// type of target node
                        channel_type = nn_to_c(n_type, target_type);// type of channel
                        augment_cumulative(int_net[source].node_get_port_load(port),
                                           rs.channel_load[channel_type].cumulatives);
                    }
                }
            }
        }
    }
    for (size_t n_type = 0; n_type < System::num_node_types; ++n_type)// sum over all node types
    {
        augment_cumulative(rs.node_load[n_type].cumulatives,
                           rs.node_load[System::num_node_types].cumulatives);
    }
    for (size_t c_type = 0; c_type < System::num_channel_types; ++c_type)// sum over all channel types
    {
        augment_cumulative(rs.channel_load[c_type].cumulatives,
                           rs.channel_load[System::num_channel_types].cumulatives);
    }
}

void compute_load_stats(// supplied/computed data:
                        RoutingStats&    rs)
/*
With the load data 'rs.node/channel_load[?].cumulatives' in 'rs', compute the corresponding stats.
Note that the stats are over all healthy nodes (including nodes of load 0).
*/
{
    for (size_t n_type = 0; n_type <= System::num_node_types; ++n_type)// over all node types and overall
    {
        rs.node_load[n_type].number = compute_cumulative_number(rs.node_load[n_type].cumulatives);
                                                                       // = num. of healthy 'n_type' nodes
        rs.node_load[n_type].sum = compute_cumulative_sum(rs.node_load[n_type].cumulatives);
        rs.node_load[n_type].average = compute_cumulative_average(rs.node_load[n_type].sum,
                                                                  rs.node_load[n_type].number);
        rs.node_load[n_type].std_dev = compute_cumulative_std_dev(rs.node_load[n_type].cumulatives,
                                                                  rs.node_load[n_type].average,
                                                                  rs.node_load[n_type].number);
        rs.node_load[n_type].coeff_var = compute_cumulative_coeff_var(rs.node_load[n_type].average,
                                                                      rs.node_load[n_type].std_dev);
        rs.node_load[n_type].median = compute_cumulative_median(rs.node_load[n_type].cumulatives,
                                                                rs.node_load[n_type].number);
        rs.node_load[n_type].minimum = compute_cumulative_minimum(rs.node_load[n_type].cumulatives);
        rs.node_load[n_type].maximum = compute_cumulative_maximum(rs.node_load[n_type].cumulatives);
    }
    for (size_t c_type = 0; c_type <= System::num_channel_types; ++c_type)// over ch. types and overall
    {
        rs.channel_load[c_type].number = compute_cumulative_number(rs.channel_load[c_type].cumulatives);
                                                                   // = num. of healthy 'c_type' channels
        rs.channel_load[c_type].sum = compute_cumulative_sum(rs.channel_load[c_type].cumulatives);
        rs.channel_load[c_type].average = compute_cumulative_average(rs.channel_load[c_type].sum,
                                                                     rs.channel_load[c_type].number);
        rs.channel_load[c_type].std_dev = compute_cumulative_std_dev(rs.channel_load[c_type].cumulatives,
                                                                     rs.channel_load[c_type].average,
                                                                     rs.channel_load[c_type].number);
        rs.channel_load[c_type].coeff_var = compute_cumulative_coeff_var(rs.channel_load[c_type].average,
                                                                         rs.channel_load[c_type].std_dev);
        rs.channel_load[c_type].median = compute_cumulative_median(rs.channel_load[c_type].cumulatives,
                                                                   rs.channel_load[c_type].number);
        rs.channel_load[c_type].minimum = compute_cumulative_minimum(rs.channel_load[c_type].cumulatives);
        rs.channel_load[c_type].maximum = compute_cumulative_maximum(rs.channel_load[c_type].cumulatives);
    }
}

void collect_batch_load_data(// supplied data:
                             const size_t                   start_tp,
                             const size_t                   end_tp,
                             const vector<RoutingStats>&    tp_stats,
                             // computed data:
                             RoutingStats&                  batch_tp)
/*
Collect the batch load data from the tps in 'tp_stats' where the batch goes from tp 'start_tp' to
'end_tp' - 1.
*/
{
    for (size_t nt = 0; nt <= System::num_node_types; ++nt)
    {
        batch_tp.node_load[nt].cumulatives.clear();
        for (size_t tp = start_tp; tp < end_tp; ++tp)
        {
            augment_cumulative(tp_stats[tp].node_load[nt].cumulatives,
                               batch_tp.node_load[nt].cumulatives);
        }
    }
    for (size_t ct = 0; ct <= System::num_channel_types; ++ct)
    {
        batch_tp.channel_load[ct].cumulatives.clear();
        for (size_t tp = start_tp; tp < end_tp; ++tp)
        {
            augment_cumulative(tp_stats[tp].channel_load[ct].cumulatives,
                               batch_tp.channel_load[ct].cumulatives);
        }
    }
}

/**
************************************************
Collect activity data and compute activity stats
************************************************
**/

void collect_activity_data(// supplied data:
                           const vector<node>&    int_net,
                           const size_t           tp_number,
                           // computed data:
                           RoutingStats&          rs)
/*
After a tp-run, a node/port will have a map mu that is defined at t as x_t only if:
 - x_t > 0 is the number of activities of the node/port at time t.
with the total number of activities of the node/port being given by:
 - x_t1 + x_t2 + ... + x_tT where the time-slots where the node/port is active at least once
   are t1, t2, ..., tT.
In particular, if the node/port has no activities at time t then mu is undefined on t.

We collect all the timed activity data from a tp-run within a 'routing_stats' class object. Note that
we only collect activities of healthy components. If we have 'routing_stats X' then we have, e.g.:
 - X.node_timed_activity[G.pr], X.node_timed_activity[G.sw], X.node_timed_activity[*], ...
and for, e.g., X.node_timed_activity[G.pr] we have:
 - X.node_timed_activity[G.pr].cumulatives[t] = (0, n_1, ..., n_i, ..., n_Rt) for 0 <= t <= T_pr where
    - T_pr = largest time-slot appearing amongst activity maps for processor nodes
    - Rt = largest activity load appearing in time-slot t for processor nodes
    - n_i = number of processors with t-activity load i > 0
       - so Sigma(n_i) = number of healthy processors with non-zero t-activity load
    - note that the 0th entry is 0 as we don't collect data for time-slots with 0 activity loads.
However, we get "n_0" by using the total number of healthy nodes in the tp indexed by 'tp_number' and
subtracting Sigma(n_i), below. So, for a tp-run we end up having, e.g.,
 - X.node_timed_activity[G.pr].cumulatives[t] = (n_0, n_1, ..., n_i, ..., n_Rt) where
    - Sigma(n_i) is now the number of healthy nodes.
Note that to get, e.g., X.node_timed_activity[G.pr].average = (m_0, m_1, ..., m_T_pr) where
 - m_t = average activity load in time-slot t for processor nodes, etc.,
we need to divide by the total number of healthy nodes which is indeed Sigma(n_i) as just explained.

We also collect overall node activities in the cumulative vectors, e.g., 'rs.node_activity[pr].cumulatives'.
*/
{
    for (size_t n_type = 0; n_type < System::num_node_types; ++n_type)// over all node types
    {
        for (size_t source = G.node_types[n_type].first; source < G.node_types[n_type].last; ++source)
        {                                                             // over all nodes of a given type
            if (int_net[source].node_get_health() == System::healthy)// only collect data for healthy nodes
            {
                size_t overall_activity = 0;        // number of activities for the node
                const auto& loads = int_net[source].node_get_act_load();
                for (const auto& [t, act] : loads)  // log each non-zero activity load
                {
                    overall_activity += act;        // add to number of activities for the node
                    if (t >= rs.node_timed_activity[n_type].cumulatives.size())// resize if big time-slot
                        rs.node_timed_activity[n_type].cumulatives.resize(t + 1, vector<size_t>{});
                    augment_cumulative(act, rs.node_timed_activity[n_type].cumulatives[t]);
                }                        // log in 'cumulatives': note that no act. loads = 0 are logged!
                augment_cumulative(overall_activity, rs.node_activity[n_type].cumulatives);
                            // log in overall 'cumulatives': note that overall act. loads = 0 are logged
                size_t num_ports = int_net[source].node_get_num_ports();
                for (size_t port = 0; port < num_ports; ++port) // over all ports ditto
                {
                    if (int_net[source].node_get_port_health(System::out, port) == System::healthy)
                    {
                        size_t overall_activity = 0;
                        size_t target = int_net[source].node_get_port_node(System::out, port);
                        size_t t_type = int_net[target].node_get_type();
                        size_t c_type = nn_to_c(n_type, t_type);
                        const auto& loads = int_net[source].node_get_port_act_load(port);
                        for (const auto& [t, act] : loads)
                        {
                            overall_activity += act;
                            if (t >= rs.channel_timed_activity[c_type].cumulatives.size())
                                rs.channel_timed_activity[c_type].cumulatives.resize(t + 1,
                                                                                     vector<size_t>{});
                            augment_cumulative(act, rs.channel_timed_activity[c_type].cumulatives[t]);
                        }
                        augment_cumulative(overall_activity, rs.channel_activity[c_type].cumulatives);
                    }
                }
            }
        }
    }

    // add in 0-components using fault data

    for (size_t n_type = 0; n_type < System::num_node_types; ++n_type) // over all node types
    {
        size_t num_time_slots = rs.node_timed_activity[n_type].cumulatives.size(); // # possible time-slots
        for (size_t t = 0; t < num_time_slots; ++t)// over all time-slots
        {
            size_t size_for_time_t = rs.node_timed_activity[n_type].cumulatives[t].size();// time-t tuple
            size_t total_nodes_with_activities = 0;//number of nodes with activity load > 0
            for (size_t act = 1; act < size_for_time_t; ++act)// for every possible non-zero activity
            {                                                 //  load at time t
                total_nodes_with_activities += rs.node_timed_activity[n_type].cumulatives[t][act];
            }
            if (size_for_time_t > 0)// make sure there is a 0-time-slot, calculate "n_0" and register it
            {
                size_t num_healthy_nodes = G.node_types[n_type].number - TP.expanded[tp_number].num_faulty_nodes[n_type];
                rs.node_timed_activity[n_type].cumulatives[t][0] = num_healthy_nodes
                                                                        - total_nodes_with_activities;
            }
        }
    }

    // collect cumulatives overall activity loads over all nodes/channels

    for (size_t n_type = 0; n_type < System::num_node_types; ++n_type)
    {
        augment_cumulative(rs.node_activity[n_type].cumulatives,
                           rs.node_activity[System::num_node_types].cumulatives);
    }
    for (size_t c_type = 0; c_type < System::num_channel_types; ++c_type)
    {
        augment_cumulative(rs.channel_activity[c_type].cumulatives,
                           rs.channel_activity[System::num_channel_types].cumulatives);
    }

    // collect cumulatives timed activity loads over all nodes/channels

    for (size_t n_type = 0; n_type < System::num_node_types; ++n_type)
    {
        size_t max_t = rs.node_timed_activity[n_type].cumulatives.size();
        if (max_t > rs.node_timed_activity[System::num_node_types].cumulatives.size())
            rs.node_timed_activity[System::num_node_types].cumulatives.resize(max_t, vector<size_t>{});
        for (size_t t = 0; t < max_t; ++t)
        {
            augment_cumulative(rs.node_timed_activity[n_type].cumulatives[t],
                               rs.node_timed_activity[System::num_node_types].cumulatives[t]);
        }
    }
    for (size_t c_type = 0; c_type < System::num_channel_types; ++c_type)
    {
        size_t max_t = rs.channel_timed_activity[c_type].cumulatives.size();
        if (max_t > rs.channel_timed_activity[System::num_channel_types].cumulatives.size())
            rs.channel_timed_activity[System::num_channel_types].cumulatives.resize(max_t,
                                                                                    vector<size_t>{});
        for (size_t t = 0; t < max_t; ++t)
        {
            augment_cumulative(rs.channel_timed_activity[c_type].cumulatives[t],
                               rs.channel_timed_activity[System::num_channel_types].cumulatives[t]);
        }
    }
}

void compute_activity_stats(// supplied/computed data:
                            RoutingStats&    rs)
/*
Having collected the timed activity data in 'rs', compute the corresponding stats.
*/
{
    for (size_t n_type = 0; n_type <= System::num_node_types; ++n_type)
    {
        rs.node_activity[n_type].number = compute_cumulative_number(rs.node_activity[n_type].cumulatives);
        rs.node_activity[n_type].sum = compute_cumulative_sum(rs.node_activity[n_type].cumulatives);
        rs.node_activity[n_type].average = compute_cumulative_average(rs.node_activity[n_type].sum,
                                                                      rs.node_activity[n_type].number);
        rs.node_activity[n_type].std_dev = compute_cumulative_std_dev(rs.node_activity[n_type].cumulatives,
                                                                      rs.node_activity[n_type].average,
                                                                      rs.node_activity[n_type].number);
        rs.node_activity[n_type].coeff_var = compute_cumulative_coeff_var(rs.node_activity[n_type].average,
                                                                          rs.node_activity[n_type].std_dev);
        rs.node_activity[n_type].median = compute_cumulative_median(rs.node_activity[n_type].cumulatives,
                                                                    rs.node_activity[n_type].number);
        rs.node_activity[n_type].minimum = compute_cumulative_minimum(rs.node_activity[n_type].cumulatives);
        rs.node_activity[n_type].maximum = compute_cumulative_maximum(rs.node_activity[n_type].cumulatives);
        rs.node_timed_activity[n_type].number.clear();
        rs.node_timed_activity[n_type].sum.clear();
        rs.node_timed_activity[n_type].average.clear();
        rs.node_timed_activity[n_type].std_dev.clear();
        rs.node_timed_activity[n_type].coeff_var.clear();
        rs.node_timed_activity[n_type].median.clear();
        rs.node_timed_activity[n_type].minimum.clear();
        rs.node_timed_activity[n_type].maximum.clear();
        for (size_t t = 0; t < rs.node_timed_activity[n_type].cumulatives.size(); ++t)
        {
            rs.node_timed_activity[n_type].number.push_back(compute_cumulative_number(
                    rs.node_timed_activity[n_type].cumulatives[t]));
            rs.node_timed_activity[n_type].sum.push_back(compute_cumulative_sum(
                    rs.node_timed_activity[n_type].cumulatives[t]));
            rs.node_timed_activity[n_type].average.push_back(compute_cumulative_average(
                    rs.node_timed_activity[n_type].sum[t],
                    rs.node_timed_activity[n_type].number[t]));
            rs.node_timed_activity[n_type].std_dev.push_back(compute_cumulative_std_dev(
                    rs.node_timed_activity[n_type].cumulatives[t],
                    rs.node_timed_activity[n_type].average[t],
                    rs.node_timed_activity[n_type].number[t]));
            rs.node_timed_activity[n_type].coeff_var.push_back(compute_cumulative_coeff_var(
                    rs.node_timed_activity[n_type].average[t],
                    rs.node_timed_activity[n_type].std_dev[t]));
            rs.node_timed_activity[n_type].median.push_back(compute_cumulative_median(
                    rs.node_timed_activity[n_type].cumulatives[t],
                    rs.node_timed_activity[n_type].number[t]));
            rs.node_timed_activity[n_type].minimum.push_back(compute_cumulative_minimum(
                    rs.node_timed_activity[n_type].cumulatives[t]));
            rs.node_timed_activity[n_type].maximum.push_back(compute_cumulative_maximum(
                    rs.node_timed_activity[n_type].cumulatives[t]));
        }
    }
    for (size_t c_type = 0; c_type <= System::num_channel_types; ++c_type)
    {
        rs.channel_activity[c_type].number = compute_cumulative_number(
                    rs.channel_activity[c_type].cumulatives);
        rs.channel_activity[c_type].sum = compute_cumulative_sum(
                    rs.channel_activity[c_type].cumulatives);
        rs.channel_activity[c_type].average = compute_cumulative_average(
                    rs.channel_activity[c_type].sum, rs.channel_activity[c_type].number);
        rs.channel_activity[c_type].std_dev = compute_cumulative_std_dev(
                    rs.channel_activity[c_type].cumulatives, rs.channel_activity[c_type].average,
                    rs.channel_activity[c_type].number);
        rs.channel_activity[c_type].coeff_var = compute_cumulative_coeff_var(
                    rs.channel_activity[c_type].average, rs.channel_activity[c_type].std_dev);
        rs.channel_activity[c_type].median = compute_cumulative_median(
                    rs.channel_activity[c_type].cumulatives, rs.channel_activity[c_type].number);
        rs.channel_activity[c_type].minimum = compute_cumulative_minimum(
                    rs.channel_activity[c_type].cumulatives);
        rs.channel_activity[c_type].maximum = compute_cumulative_maximum(
                    rs.channel_activity[c_type].cumulatives);
        rs.channel_timed_activity[c_type].number.clear();
        rs.channel_timed_activity[c_type].sum.clear();
        rs.channel_timed_activity[c_type].average.clear();
        rs.channel_timed_activity[c_type].std_dev.clear();
        rs.channel_timed_activity[c_type].coeff_var.clear();
        rs.channel_timed_activity[c_type].median.clear();
        rs.channel_timed_activity[c_type].minimum.clear();
        rs.channel_timed_activity[c_type].maximum.clear();
        for (size_t t = 0; t < rs.channel_timed_activity[c_type].cumulatives.size(); ++t)
        {
            size_t value_s = compute_cumulative_number(rs.channel_timed_activity[c_type].cumulatives[t]);
            rs.channel_timed_activity[c_type].number.push_back(value_s);
            value_s = compute_cumulative_sum(rs.channel_timed_activity[c_type].cumulatives[t]);
            rs.channel_timed_activity[c_type].sum.push_back(value_s);
            double value_d = compute_cumulative_average(rs.channel_timed_activity[c_type].sum[t],
                                                        rs.channel_timed_activity[c_type].number[t]);
            rs.channel_timed_activity[c_type].average.push_back(value_d);
            value_d = compute_cumulative_std_dev(rs.channel_timed_activity[c_type].cumulatives[t],
                                                 rs.channel_timed_activity[c_type].average[t],
                                                 rs.channel_timed_activity[c_type].number[t]);
            rs.channel_timed_activity[c_type].std_dev.push_back(value_d);
            value_d = compute_cumulative_coeff_var(rs.channel_timed_activity[c_type].average[t],
                                                   rs.channel_timed_activity[c_type].std_dev[t]);
            rs.channel_timed_activity[c_type].coeff_var.push_back(value_d);
            value_d = compute_cumulative_median(rs.channel_timed_activity[c_type].cumulatives[t],
                                              rs.channel_timed_activity[c_type].number[t]);
            rs.channel_timed_activity[c_type].median.push_back(value_d);
            value_s = compute_cumulative_minimum(rs.channel_timed_activity[c_type].cumulatives[t]);
            rs.channel_timed_activity[c_type].minimum.push_back(value_s);
            value_s = compute_cumulative_maximum(rs.channel_timed_activity[c_type].cumulatives[t]);
            rs.channel_timed_activity[c_type].maximum.push_back(value_s);
        }
    }
}

void collect_batch_activity_data(// supplied data:
                                 const size_t                    start_tp,
                                 const size_t                    end_tp,
                                 const vector<RoutingStats>&     tp_stats,
                                 // computed data:
                                 RoutingStats&                   batch_tp)
/*
Collect the batch activity load data from the tps in 'tp_stats' where the batch goes from tp 'start_tp'
to 'end_tp' - 1.
*/
{
    for (size_t nt = 0; nt <= System::num_node_types; ++nt)
    {
        batch_tp.node_activity[nt].cumulatives.clear();
        for (size_t tp = start_tp; tp < end_tp; ++tp)
        {
            augment_cumulative(tp_stats[tp].node_activity[nt].cumulatives,
                               batch_tp.node_activity[nt].cumulatives);
        }
    }
    for (size_t ct = 0; ct <= System::num_channel_types; ++ct)
    {
        batch_tp.channel_activity[ct].cumulatives.clear();
        for (size_t tp = start_tp; tp < end_tp; ++tp)
        {
            augment_cumulative(tp_stats[tp].channel_activity[ct].cumulatives,
                               batch_tp.channel_activity[ct].cumulatives);
        }
    }

    for (size_t nt = 0; nt <= System::num_node_types; ++nt)
    {
        batch_tp.node_timed_activity[nt].cumulatives.clear();
        for (size_t tp = start_tp; tp < end_tp; ++tp)
        {
            size_t num_time_slots = tp_stats[tp].node_timed_activity[nt].cumulatives.size();
            if (batch_tp.node_timed_activity[nt].cumulatives.size() < num_time_slots)
                batch_tp.node_timed_activity[nt].cumulatives.resize(num_time_slots, vector<size_t>{});
            for (size_t t = 0; t < num_time_slots; ++t)
            {
                augment_cumulative(tp_stats[tp].node_timed_activity[nt].cumulatives[t],
                                   batch_tp.node_timed_activity[nt].cumulatives[t]);
            }
        }
    }
    for (size_t ct = 0; ct <= System::num_channel_types; ++ct)
    {
        batch_tp.channel_timed_activity[ct].cumulatives.clear();
        for (size_t tp = start_tp; tp < end_tp; ++tp)
        {
            size_t num_time_slots = tp_stats[tp].channel_timed_activity[ct].cumulatives.size();
            if (batch_tp.channel_timed_activity[ct].cumulatives.size() < num_time_slots)
                batch_tp.channel_timed_activity[ct].cumulatives.resize(num_time_slots, vector<size_t>{});
            for (size_t t = 0; t < num_time_slots; ++t)
            {
                augment_cumulative(tp_stats[tp].channel_timed_activity[ct].cumulatives[t],
                                   batch_tp.channel_timed_activity[ct].cumulatives[t]);
            }
        }
    }
}
