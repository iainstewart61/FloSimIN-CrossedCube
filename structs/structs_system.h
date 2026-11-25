
#ifndef STRUCTS_SYSTEM_H
#define STRUCTS_SYSTEM_H

#include <cstddef>
#include <array>
#include <utility>

#include "structs_node_channel.h"
#include "../errors/error_writing.h"

/**
***********************************
System structs and helper functions
***********************************
**/

struct System
/*
A struct to describe the system globals. A global variable 'G' will be defined via:
 - System G;
*/
{
    // all terminal node types come first
    static constexpr std::size_t num_term_types = 2;// number of terminal node types (which now follow)
    static constexpr std::size_t pr = 0;    // the index of processors in node_types (below)
    static constexpr std::size_t se = 1;    // the index of servers in node_types (below)
    // before all non-terminal node types
    static constexpr std::size_t num_switch_types = 2;// the number of non-terminal types (which now follow)
    static constexpr std::size_t sw = 2;    // the index of switches in node_types (below)
    static constexpr std::size_t os = 3;    // the index of optical switches in node_types (below)

    static constexpr std::size_t num_node_types = num_term_types + num_switch_types; // number of node types
    static constexpr std::size_t num_channel_types = num_node_types * num_node_types;// number of channel types
    //  with the ordering (0,0), ..., (0, 'num_node_types' - 1), (1, 0), ..., ('num_node_types' - 1,
    //  'num_node_types' - 1) in rightmost l.s.b. lexicographic order
    static constexpr size_t in = 1;			// port is an input port
    static constexpr size_t out = 0;	    // port is an output port
    static constexpr int used = 0;          // node/port used (might in future prioritize as 0,1,2,...)
    static constexpr int unused = -1;       // node/port unused (needs to be < 0; see future use)
    static constexpr bool healthy = false;  // node/port healthy
    static constexpr bool faulty = true;    // node/port faulty
    static constexpr std::size_t dp_precision = 2;// number of decimal places results are written as throughout
    static constexpr std::size_t num_seeds = 8; // number of seeds for a random number generator

    // populated in "initialize_system_globals()"

    std::array<NodeType, num_node_types> node_types;          // list of node types
    std::array<ChannelType, num_channel_types> channel_types; // list of channel types

    // populated in "topology_numerics()"

    std::size_t num_terms{};      // G.node_types[System::pr].number + G.node_types[System::se].number
    std::size_t num_switches{};   // G.node_types[System::sw].number + G.node_types[System::os].number
    std::size_t total_nodes{};    // G.num_terms + G.num_switches
    std::size_t total_channels{}; // SUM_{i}(G.channel_types[i].number)

    // globals to be used when debugging and so not available to the user (normally all false)

    static constexpr bool view_system = false;     // view all of the system variables (to screen)
    static constexpr bool view_registry = false;   // view the full contents of the registry (to screen)
    size_t global_size_t;                          // a size_t to use when debugging
};

constexpr std::size_t nn_to_c(std::size_t id1, std::size_t id2) noexcept
/*
Helper function to associate a channel id with a given pair of node ids.

Assumptions:
 - 'id1' and 'id2' are legal, i.e., 'id1', 'id2' in {0, 1, 2, 3}.
*/
{
    if (!(id1 < System::num_node_types) || !(id2 < System::num_node_types))
        coding(Errors::OutOfRangeNodeType);
    return id1 * System::num_node_types + id2;
}

constexpr std::pair<std::size_t,std::size_t> c_to_nn(std::size_t ch) noexcept
/*
Helper function to associate a pair of node ids with a given channel id.

Assumptions:
 - 'ch' is legal, i.e., 'ch' in {0, 1, ..., 15}.
*/
{
    if (!(ch < System::num_channel_types))
        coding(Errors::OutOfRangeNodeType);
    return {ch / System::num_node_types, ch % System::num_node_types};
}

#endif
