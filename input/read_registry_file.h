
#ifndef READ_REGISTRY_FILE_H
#define READ_REGISTRY_FILE_H

#include <cstddef>
#include <string>
#include <vector>

#include "data_structures_for_config.h"
#include "../structs/structs_registry.h"

enum class ReadMode : std::size_t
/*
An enum used when we are reading the registry in order to denote which tag we are currently dealing with.
*/
{
    no_read, int_net, traffic_pattern, note, end_of_file
};

struct RegIntNet
/*
A struct holding all the data read from the registry in relation to an interconnection network.
*/
{
    std::string                 name;
    std::vector<std::string>    parameters;
    RegistryNetMachine          net_machine;
    RegistryNetType             net_type;
    RegistryNetBuild            net_build;
    std::vector<std::string>    routing_algs;
};

struct RegNote
/*
A struct holding all the data read from the registry in relation to a note for an interconnection network.
*/
{
    std::string    name;
    std::string    note;
};

struct TheRegistry
/*
A struct holding the entire contents of the registry. This is used for validation and also for re-writing the
registry in a clean-up fashion.
*/
{
    std::vector<RegIntNet>      int_nets_registered{};
    std::vector<std::string>    traffic_patterns_registered{};
    std::vector<RegNote>        notes_registered{};
};

/**
****************************************************************
Interconnection network/routing algorithm registration functions
****************************************************************
**/

/**
**************************************
Traffic pattern registration functions
**************************************
**/

/**
***************************
Note registration functions
***************************
**/

/**
*****************
Read the registry
*****************
**/

void read_int_net_data_from_registry(// supplied data:
                                     const std::string&                    input_file_name,
                                     const std::vector<ValueAndTriple>&    values_and_triples,
                                     // computed data:
                                     TheRegistry&                          the_registry,
                                     std::string&                          pre_registry,
                                     std::string&                          post_registry,
                                     // other stuff:
                                     std::ostream&                         output);

#endif
