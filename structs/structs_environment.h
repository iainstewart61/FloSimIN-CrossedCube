
#ifndef STRUCTS_ENVIRONMENT_H
#define STRUCTS_ENVIRONMENT_H

#include <cstddef>

/**
*****************************
Environment enums and structs
*****************************
**/

enum class ViewRoutes : std::size_t
/*
An enum to register values for the global variable 'ENV.view_routes'. The default value is held in
'list_of_config_tags'.
*/
{
    all, bad, none
};

enum class ViewGlobals : std::size_t
/*
An enum to register values for the global variable 'ENV.view_output'. The default value is held in
'list_of_config_tags'.
*/
{
    none, all, nondef
};

struct Environment
/*
A struct to describe all parameters associated with the environment. A global variable 'ENV' will be defined
via:
 - Environment ENV;
Any default values are held in 'config_tags'.
*/
{
    bool           sync;                     // denotes whether synchronous data is to be collected
    std::size_t    sync_worm;                // the length (> 0) of the worm in synchronous loads
    bool           suppress_tps;              // true = don't write individual tp data to output file
    bool           use_used_mode;            // used mode = it matters whether used or unused
    ViewRoutes     view_routes;              // "all" = display all, "bad" = only bad routes, "none" = none
    bool           view_int_net;             // true = display int net topology, false = don't
    bool           view_int_net_with_faults; // true = display int. net. with faults, false = don't
    bool           view_loads;               // true = if viewing int net, view loads as well
    ViewGlobals    view_globals;             // view globals on screen/in file: "none" = no; "all" = all
                                             //  globals; "nondef" = non-defaults only
    bool           rewrite_registry;         // whether to rewrite the registry file
};

#endif
