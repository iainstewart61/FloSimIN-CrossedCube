
#ifndef STRUCTS_REGISTRY_H
#define STRUCTS_REGISTRY_H

#include <cstddef>
#include <string>

/**
**************************
Registry enums and structs
**************************
**/

enum class RegistryNetMachine : std::size_t
{
    supercom, dcn
};

enum class RegistryNetType : std::size_t
{
    cartesian, permutation
};

enum class RegistryNetBuild : std::size_t
/*
Let (node, output port) (x, o_x) be associated with (y, i_y) and let (y, o_y) be associated with (x, i_x).
 - symmetric means o_x = i_y = o_y = i_x
 - partsymmetric means o_x = i_x and o_y = i_y
 - nonsymmetric means otherwise.
*/
{
    symmetric, partsymmetric, nonsymmetric
};

struct RegistryGlobals
/*
A struct to describe all parameters that are read from the registry. A global variable 'RG' will be defined
via:
 - RegistryGlobals RG;
Each of these parameters needs to be read from the registry (so there are no defaults).
*/
{
    RegistryNetMachine    net_machine;        // the interconnection network machine
    RegistryNetType       net_type;           // the interconnection network type
    RegistryNetBuild      net_build;          // whether the int. net. has been symmetrically built
    std::string           registry_note = ""; // a note in the registry about the network
};

#endif
