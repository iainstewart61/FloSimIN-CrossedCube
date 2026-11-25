
#ifndef HYPERCUBE_ROUTING_H
#define HYPERCUBE_ROUTING_H

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

void hypercube_routing_prelude();

void hypercube_dor_routing(// supplied data:
                           std::vector<node>&      int_net,
                           const std::size_t       source,
                           const std::size_t       destination,
                           // computed data:
                           struct RouteStruct&     route);

#endif
