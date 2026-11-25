
#ifndef MAPS_H
#define MAPS_H

#include <unordered_map>
#include <string>

#include "structs/structs_map.h"

void setup_all_maps(std::unordered_map<std::string, Maps::IntNetGlobalsFn>&     int_net_globals,
                    std::unordered_map<std::string, Maps::IntNetBuildFn>&       int_net_build,
                    std::unordered_map<std::string, Maps::RoutingAlgFn>&        routing_algs,
                    std::unordered_map<std::string, Maps::RoutingPreludeFn>&    routing_preludes,
                    std::unordered_map<std::string, Maps::TrafficPatternFn>&    traffic_patterns,
                    std::unordered_map<std::string, Maps::AlgToNumericFn>&      alg_to_numeric,
                    std::unordered_map<std::string, Maps::AlgFromNumericFn>&    alg_from_numeric);

#endif
