
#ifndef DATA_LOGGING_H
#define DATA_LOGGING_H

#include <vector>
#include <cstddef>

#include "../node.h"
#include "../structs/structs_routing.h"

void augment_cumulative(// supplied data:
                        const std::size_t            to_add,
                        // supplied/computed data:
                        std::vector<std::size_t>&    cumulative);

void augment_cumulative(// supplied data:
                        const std::vector<std::size_t>&    to_add,
                        // supplied/computed data:
                        std::vector<std::size_t>&          cumulative);

void reset_all_loads(// supplied data:
                     std::vector<node>&    int_net);

void reset_all_sync_loads(// supplied data:
                          std::vector<node>&    int_net);

void route_log_basic_latency(// supplied data:
                             const RouteStruct&           route,
                             // supplied/computed data:
                             std::size_t&                 failed_routes,
                             std::size_t&                 successful_routes,
                             std::vector<std::size_t>&    cumulative_hop_lengths,
                             std::size_t&                 times_tried);

void route_log_basic_loads(// supplied data:
                           const struct RouteStruct&          route,
                           //supplied/computed data:
                           std::vector<node>&                 int_net);

#endif
