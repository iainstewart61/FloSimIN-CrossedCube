
#ifndef STRUCTS_BANDWIDTH_H
#define STRUCTS_BANDWIDTH_H

#include <vector>
#include <cstddef>

/**
*****************
Bandwidth structs
*****************
**/

struct Bandwidth
/*
A struct to describe all parameters associated with bandwidth. A global variable 'BAN' will be defined via:
 - Bandwidth BAN;
Any default values are held in 'config_tags'.
*/
{
    std::vector<std::size_t>    port_bandwidth; // chan. bandw. (in bits/sec) of 16 different chan. types
};

#endif
