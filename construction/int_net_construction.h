
#ifndef INT_NET_CONSTRUCTION_H
#define INT_NET_CONSTRUCTION_H

#include <ostream>
#include <vector>
#include <cstddef>
#include <utility>
#include <map>
#include <optional>

#include "../node.h"
#include "../structs/structs_routing.h"

/**
********************************************************
Functions relating to cartesian <-> algebraic conversion
********************************************************
**/

std::size_t cart_alg_convert_to_id(// supplied data:
                                   const std::vector<std::size_t>&    the_alg_id,
                                   const std::vector<std::size_t>&    dim_bounds);

std::vector<std::size_t> cart_alg_convert_from_id(// supplied data:
                                                  const std::size_t                  the_relative_id,
                                                  const std::vector<std::size_t>&    dim_bounds);

                                                  /**
*****************************************************
Functions relating to channel and route manipulations
*****************************************************
**/

void get_channel_type_and_target_id(// supplied data:
                                    const std::vector<node>&    int_net,
                                    const std::size_t           source_abs_id,
                                    const std::size_t           source_port,
                                    // computed data:
                                    std::size_t&                the_channel_type,
                                    std::size_t&                target_abs_id);

std::pair<std::pair<std::optional<std::size_t>, std::optional<std::size_t>>,
std::pair<std::optional<std::size_t>,
std::optional<std::size_t>>> ports_between_nodes(//supplied data:
                                                 const std::vector<node>&    int_net,
                                                 const std::size_t           source_node,
                                                 const std::size_t           target_node);

RouteError fault_check_route(//supplied data:
                              const std::vector<node>&      int_net,
                              const size_t                  source,
                              const size_t                  destination,
                              const struct RouteStruct&     route);

/**
**********************************************************
Functions relating to interconnection network construction
**********************************************************
**/

void validate_int_net_globals();

void topology_numerics(// other stuff:
                       std::ostream&    output);

void prelude_to_int_net_build(// supplied/computed data:
                              std::vector<node>&    int_net);

void postlude_to_int_net_build();

#endif
