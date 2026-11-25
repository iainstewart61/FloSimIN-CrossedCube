
#ifndef STRUCTS_NODE_CHANNEL_H
#define STRUCTS_NODE_CHANNEL_H

#include <string>
#include <cstddef>
#include <vector>

/**
************************
Node and channel structs
************************
**/

struct NodeType
/*
A struct to describe the information associated with each node type. We regard each node type as a
 - size_t in {0, 1, ..., System::num_node_types - 1}.
All terminal node types need to come before all non-terminal (a.k.a. switch) node types in 'G.node_types'
and 'G.node_types' is initialized in initialize_system_globals().
*/
{
    std::string                 name = "";          // name of a node type
                                                    // (set in initialize_system_globals())
    std::string                 short_name = "";    // 2-char name of a node type
                                                    // (set in initialize_system_globals())
    bool                        terminal = true;    // is a terminal type
                                                    // (set in initialize_system_globals())
    std::size_t                 number = 0;         // number of nodes of this type
                                                    // (set in <int_net>_set_globals())
    std::size_t                 alg_rep = 0;        // length of alg rep of a node
                                                    // (set in <int_net>_set_globals())
    std::vector<std::size_t>    alg_rep_range{};    // ranges of alg rep of node
                                                    // (set in <int_net>_set_globals())
    std::size_t                 first = 0;          // abs. id of the first node of this type
                                                    // (set in topology_numerics())
    std::size_t                 last = 0;           // 1+abs. id of the last node of this type
                                                    // (set in topology_numerics())
    std::size_t                 pad_abs = 0;        // padding for writing abs ids
                                                    // (set in compute_pad_values_for_writing())
    std::size_t                 pad_rel = 0;        // padding for writing rel ids
                                                    // (set in compute_pad_values_for_writing())
    std::size_t                 pad_bit = 0;        // padding for writing alg rep
                                                    // (set in compute_pad_values_for_writing())
};

struct ChannelType
/*
A struct to describe the information associated with each channel type. We regard each channel as a
 - size_t in {0, 1, ..., System::num_channel_types - 1}.
It will always be the case that there is a channel type for every ordered pair of node types. So, if there
are x node types then there are x^2 channel types with the bijections:
 - "nn_to_c()" between pairs of node types and channel types
 - "c_to_nn()" between channel types and pairs of node types
both defined in "structs_system.h". The channel types are in 'G.channel_types' and initialized in
initialize_system_globals().
*/
{
    std::string    name = "";           // name of a channel type
    std::string    short_name = "";     // short name of a channel type (4 characters)
    std::size_t    number = 0;          // number of channels of this type (set in "<int_net>_set_globals()")
};

#endif
