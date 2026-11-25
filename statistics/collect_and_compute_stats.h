
#ifndef COLLECT_AND_COMPUTE_STATS_H
#define COLLECT_AND_COMPUTE_STATS_H

#include <vector>
#include <cstddef>

#include "../structs/structs_routing_stats.h"
#include "../node.h"

/**
********************************
Collect and compute latency data
********************************
**/

void compute_latency_data(// supplied data:
                          const std::size_t                  failed_routes,
                          const std::size_t                  successful_routes,
                          const std::vector<std::size_t>&    cumulative_hop_lengths,
                          const std::size_t                  times_tried,
                          // computed data:
                          RoutingStats&                      rs);

void collect_batch_latency_data(// supplied data:
                                const std::size_t                    start_tp,
                                const std::size_t                    end_tp,
                                const std::vector<RoutingStats>&     tp_stats,
                                // computed data:
                                std::size_t&                         batch_failed_routes,
                                std::size_t&                         batch_successful_routes,
                                std::vector<std::size_t>&            batch_cumulative_hop_lengths,
                                std::size_t&                         batch_times_tried);

/**
****************************************
Collect load data and compute load stats
****************************************
**/

void collect_load_data(// supplied data:
                       const std::vector<node>&    int_net,
                       // computed data:
                       RoutingStats&               rs);

void compute_load_stats(// supplied/computed data:
                        RoutingStats&    rs);

void collect_batch_load_data(// supplied data:
                             const std::size_t                   start_tp,
                             const std::size_t                   end_tp,
                             const std::vector<RoutingStats>&    tp_stats,
                             // computed data:
                             RoutingStats&                       batch_tp);

/**
************************************************
Collect activity data and compute activity stats
************************************************
**/

void collect_activity_data(// supplied data:
                           const std::vector<node>&    int_net,
                           const std::size_t           tp_number,
                           // computed data:
                           RoutingStats&               rs);

void compute_activity_stats(// supplied/computed data:
                            RoutingStats&    rs);

void collect_batch_activity_data(// supplied data:
                                 const std::size_t                    start_tp,
                                 const std::size_t                    end_tp,
                                 const std::vector<RoutingStats>&     tp_stats,
                                 // computed data:
                                 RoutingStats&                       batch_tp);

#endif
