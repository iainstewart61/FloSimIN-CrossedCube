
#ifndef CROSSEDCUBE_ROUTING_H
#define CROSSEDCUBE_ROUTING_H

#include <vector>
#include <cstddef>

#include "../../structs/structs_routing.h"
#include "../../node.h"

/**
This header file should be identical for every interconnection network except for the prefix
in all functions consisting of the interconnection network name, e.g., "hypercube", and the
header guards '#ifndef HYPERCUBE_ROUTING_H' and '#define HYPERCUBE_ROUTING_H'.

Note that we include error_writing.h. A user can cover themselves for coding errors and should use
the general errors Errors::BuildingError and Errors::ExecutionError.
**/

void crossedcube_routing_prelude();

std::vector<std::size_t> compute_rho(// supplied data:
                                     const std::vector<std::size_t>&    current_alg,
                                     const std::vector<std::size_t>&    destination_alg,
                                     const std::size_t                  num_dbits);

void crossedcube_Efe(//supplied data:
                     std::vector<node>&     int_net,
                     const std::size_t      source,
                     const std::size_t      destination,
                     // computed data:
                     struct RouteStruct&    route);

void crossedcube_ChangSungHsu(// supplied data:
                              std::vector<node>&      int_net,
                              const std::size_t       source,
                              const std::size_t       destination,
                              // computed data:
                              struct RouteStruct&     route);

void crossedcube_Stewart(//supplied data:
                         std::vector<node>&     int_net,
                         const std::size_t      source,
                         const std::size_t      destination,
                         // computed data:
                         struct RouteStruct&    route);

void crossedcube_OtakeMouriKanedo_1(//supplied data:
                                    std::vector<node>&     int_net,
                                    const std::size_t      source,
                                    const std::size_t      destination,
                                    // computed data:
                                    struct RouteStruct&    route);

#endif
