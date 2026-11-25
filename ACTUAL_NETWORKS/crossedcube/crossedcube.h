
#ifndef CROSSEDCUBE_H
#define CROSSEDCUBE_H

#include <vector>
#include <cstddef>

#include "../../node.h"
#include "../../errors/error_writing.h"

/**
This header file should be identical for every interconnection network except for the prefix
in all functions consisting of the interconnection network name, e.g., "hypercube", and the
header guards '#ifndef HYPERCUBE_H' and '#define HYPERCUBE_H'.

Note that we include error_writing.h. A user can cover themselves for coding errors and should use
the general errors Errors::BuildingError and Errors::ExecutionError.
**/

void crossedcube_set_globals();

Errors crossedcube_alg_convert_to_id(// supplied data:
                                     const std::size_t                  node_type,
                                     const std::vector<std::size_t>&    the_alg_id,
                                     // computed data:
                                     std::size_t&                       rel_id);

Errors crossedcube_alg_convert_from_id(// supplied data:
                                       const std::size_t            node_type,
                                       const std::size_t            the_rel_id,
                                       // computed data:
                                       std::vector<std::size_t>&    the_alg_id);

void crossedcube_build(// computed data:
                       std::vector<node>&    int_net);

#endif
