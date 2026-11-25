
#ifndef STRUCTS_GLOBAL_H
#define STRUCTS_GLOBAL_H

#include <cstddef>
#include <vector>
#include <string>
#include <array>
#include <cstdint>
#include <variant>

#include "structs_system.h"
#include "structs_routing.h"
#include "structs_environment.h"
#include "structs_fault.h"
#include "structs_registry.h"

using VarVal = std::variant
/*
A type alias that holds values of specific types. It enables the default values of global variables of different types to be registered
in 'list_of_config_tags' and also allowable string values for certain global variables to be registered using 'Allowable'.
*/
<
    // related to config
    bool, int, std::size_t, std::string, std::array<std::size_t, System::num_node_types>, std::array<std::size_t,
    System::num_channel_types>, std::vector<std::size_t>, std::vector<std::uint32_t>, std::vector<std::string>, RoutingDor,
    RoutingTiebreak, RoutingMode, ViewRoutes, ViewGlobals, FaultCheck,
    // related to registry
    RegistryNetMachine, RegistryNetType, RegistryNetBuild
>;

struct TripleString
/*
A struct describing a list of three strings for map 'Allowable'.
*/
{
    std::string tag;
    std::string subtag;
    std::string value;

    auto operator<=>(const TripleString&) const = default;
};

#endif
