
#include "maps.h"
#include "traffic/traffic_generators.h"
#include "ACTUAL_NETWORKS/hypercube/hypercube.h"
#include "ACTUAL_NETWORKS/hypercube/hypercube_routing.h"
#include "ACTUAL_NETWORKS/crossedcube/crossedcube.h"
#include "ACTUAL_NETWORKS/crossedcube/crossedcube_routing.h"

using std::unordered_map;
using std::string;

/**
This ccp contains a function to build:
 - maps associating each registered interconnection network with specific functions
 - a map associating traffic pattern names with traffic pattern generators
When a new interconnection network or traffic pattern is built, there must be entries registered
in these maps.
**/

void setup_all_maps(unordered_map<string, Maps::IntNetGlobalsFn>&     int_net_globals,
                    unordered_map<string, Maps::IntNetBuildFn>&       int_net_build,
                    unordered_map<string, Maps::RoutingAlgFn>&        routing_algs,
                    unordered_map<string, Maps::RoutingPreludeFn>&    routing_preludes,
                    unordered_map<string, Maps::TrafficPatternFn>&    traffic_patterns,
                    unordered_map<string, Maps::AlgToNumericFn>&      alg_to_numeric,
                    unordered_map<string, Maps::AlgFromNumericFn>&    alg_from_numeric)
/*
Set up the maps associating interconnection networks and routing algorithms with functions.
*/
{
    // interconnection networks
    int_net_globals["hypercube"] = hypercube_set_globals;
    int_net_build["hypercube"]   = hypercube_build;
    int_net_globals["crossedcube"] = crossedcube_set_globals;
    int_net_build["crossedcube"]   = crossedcube_build;

    // routing algorithms
    routing_algs["hypercube_dor_routing"] = hypercube_dor_routing;
    routing_preludes["hypercube_dor_routing"] = hypercube_routing_prelude;
    routing_algs["crossedcube_Efe"] = crossedcube_Efe;
    routing_preludes["crossedcube_Efe"] = crossedcube_routing_prelude;
    routing_algs["crossedcube_ChangSungHsu"] = crossedcube_ChangSungHsu;
    routing_preludes["crossedcube_ChangSungHsu"] = crossedcube_routing_prelude;
    routing_algs["crossedcube_Stewart"] = crossedcube_Stewart;
    routing_preludes["crossedcube_Stewart"] = crossedcube_routing_prelude;
    routing_algs["crossedcube_OtakeMouriKanedo_1"] = crossedcube_OtakeMouriKanedo_1;
    routing_preludes["crossedcube_OtakeMouriKanedo_1"] = crossedcube_routing_prelude;

    // algorithm conversions
    alg_to_numeric["hypercube"]   = hypercube_alg_convert_to_id;
    alg_from_numeric["hypercube"] = hypercube_alg_convert_from_id;
    alg_to_numeric["crossedcube"]   = crossedcube_alg_convert_to_id;
    alg_from_numeric["crossedcube"] = crossedcube_alg_convert_from_id;

    // traffic patterns
    traffic_patterns["random"]      = random_traffic_generator;
    traffic_patterns["alltoall"]    = alltoall_traffic_generator;
    traffic_patterns["permutation"] = permutation_traffic_generator;
}


