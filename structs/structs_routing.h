
#ifndef STRUCTS_ROUTING_H
#define STRUCTS_ROUTING_H

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

#include "../errors/error_writing.h"

/**
*************************
Routing enums and structs
*************************
**/

enum class RoutingDor : std::size_t
/*
An enum to register values for the global variable 'RA.dor'. The default value is held in 'config_tags'.
*/
{
    asc, des, ran, ranperpair  // ranperpair = choose dim order randomly for every different pair
};

enum class RoutingTiebreak : std::size_t
/*
An enum to register values for the global variable 'RA.tiebreak'. The default value is held in 'config_tags'.
*/
{
    inc, dec, ran
};

enum class RoutingMode : std::size_t
/*
An enum to register values for the global variable 'RA.mode'. The default value is held in 'config_tags'.
*/
{
    det, nondet, full, incremental
};

struct RoutingAlgorithm
/*
A struct to describe the various parameters related to the input routing algorithm. A global variable 'RA'
will be defined via:
 - RoutingAlgorithm RA;
Any default values are held in 'config_tags'.
*/
{
    std::string                   name;          // the routing algorithm (a non-empty string)
    RoutingDor                    dor;           // the type of dimension order
    RoutingTiebreak               tiebreak;      // the tiebreak protocol
    RoutingMode                   mode;          // the routing execution mode
    int                           nondet_repeat; // how many non-det runs we wish to attempt (-1 = unlimited)
    int                           increm_bound;  // how many incremental turns we wish to allow (-1 = unlimited)
    bool                          valiant;       // whether to run Valiant's algorithm
    std::vector<std::uint32_t>    seeds;         // list of seeds for random num. gen. in routing algorithms
};                                               //  (System::num_seeds of them)

struct RoutingAlgorithmPrelude
/*
A struct to describe miscellaneous parameters specific to a routing algorithm and set in
"<routing_algorithm>_prelude". A global variable 'RAP' will be defined via:
 - RoutingAlgorithmPrelude RAP;
*/
{
    std::vector<std::size_t>    dim_order;        // a specific dimension order
};

struct RouteError
/*
Error description: what the error was; and where it occurred in the route.
*/
{
    Errors         code;
    std::size_t    location;                      // location in the actual route
};

struct RouteStruct
/*
This is the basic structure of a route. The size of 'port_route' gives the length of the route. If there is
no error then 'error_flag.code' = Good otherwise it can take a variety of values depending upon the error (see
error_writing() or fault_check_route()). 'times_tried' is the number of times a non-deterministic routing
algorithm running in mode "nondet" or "full" tries to find a route before either it finds a healthy route or
doesn't find one. We can get a gauge of the power of non-determinism from this number. Note that it is up to
the user to maintain this number in their routing algorithm code.
*/
{
    std::size_t                 source;
    std::vector<std::size_t>    port_route;
    RouteError                  error_flag;
    std::size_t                 times_tried = 0;
};

#endif
