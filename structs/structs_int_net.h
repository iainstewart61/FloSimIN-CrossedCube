
#ifndef STRUCTS_INT_NET_H
#define STRUCTS_INT_NET_H

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
*****************************************
Interconnection network enums and structs
*****************************************
**/

struct InterconnectionNetwork
/*
A struct to describe the core aspects of the input interconnection network. A global variable 'IN' will
be defined via:
 - InterconnectionNetwork IN;
Any default values are held in 'config_tags'.
*/
{
    std::string                   name;         // name of the network
    std::vector<std::string>      param_names;  // names of key net. params (> 0 with first always the
                                                //  dimension and usually called "n") read from config and
                                                //  needs to match exactly with param. names read from
                                                //  registry
    std::vector<std::size_t>      param_values; // key parameters for the network (dimension always the
                                                //  first and all must be > 0)
    std::vector<std::uint32_t>    seeds;        // a list of seeds for random generation for, e.g.,
                                                //  dimension orders ('System::num_seeds' of them)
};

#endif
