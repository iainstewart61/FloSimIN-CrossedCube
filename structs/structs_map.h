
#ifndef STRUCTS_MAP_H
#define STRUCTS_MAP_H

#include <string>
#include <vector>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <ostream>
#include <utility>

#include "structs_routing.h"
#include "../errors/error_writing.h"
#include "../node.h"

struct Maps
/*
The map variables to associate functions with an interconnection network IN.name. A global variable
'M' will be defined via:
 - Maps M;
*/
{
    using IntNetGlobalsFn  = std::function<void()>;
    using IntNetBuildFn    = std::function<void(std::vector<node>&)>;
    using IntNetDistance   = std::function<void(std::vector<node>&, std::size_t, std::size_t)>;
    using RoutingAlgFn     = std::function<void(std::vector<node>&, std::size_t, std::size_t,
                                                RouteStruct&)>;
    using RoutingPreludeFn = std::function<void()>;
    using TrafficPatternFn = std::function<bool(std::size_t, bool&, std::size_t&, std::size_t&,
                                                std::vector<std::pair<std::size_t, std::size_t>>&,
                                                std::size_t&)>;
    using AlgToNumericFn   = std::function<Errors(std::size_t, const std::vector<std::size_t>&,
                                                  std::size_t&)>;
    using AlgFromNumericFn = std::function<Errors(std::size_t, std::size_t, std::vector<std::size_t>&)>;

    std::unordered_map<std::string, IntNetGlobalsFn>     int_net_globals;
    std::unordered_map<std::string, IntNetBuildFn>       int_net_build;
    std::unordered_map<std::string, IntNetDistance>      int_net_distances;
    std::unordered_map<std::string, RoutingAlgFn>        routing_algs;
    std::unordered_map<std::string, RoutingPreludeFn>    routing_preludes;
    std::unordered_map<std::string, TrafficPatternFn>    traffic_patterns;
    std::unordered_map<std::string, AlgToNumericFn>      alg_to_numeric;
    std::unordered_map<std::string, AlgFromNumericFn>    alg_from_numeric;
};

#endif
