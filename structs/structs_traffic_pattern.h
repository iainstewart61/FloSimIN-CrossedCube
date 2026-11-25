
#ifndef STRUCTS_TRAFFIC_PATTERNS_H
#define STRUCTS_TRAFFIC_PATTERNS_H

#include <string>
#include <vector>
#include <cstddef>
#include <array>
#include <cstdint>
#include <variant>

#include "structs_system.h"

/**
***********************
Traffic pattern structs
***********************
**/

struct TrafficPatternSourceDest
/*
A struct to describe a source or destination description in a tp. Note the initial values. See
'TrafficPatternInterim' for more details.
*/
{
    std::string prefix = "default";     // "pr", "se", "*" or "default"
    std::variant<std::size_t, std::vector<std::size_t>> id_or_tuple;// an id or an alg id
};

struct TrafficPatternInterim
/*
A struct TrafficPatternInterim 'tp' in the vector 'TP.interim' describes a tp after it has been read from the
config file but before it has been fully validated. Note that any valid such struct will have a non-empty
name and both 'tp.num_iterations' and 'tp.num_repeats' integers > 0. Also, the 4 items in 'tp_sources_dests'
will be descriptions of the entries for start-source, end-source, start-dest and end-dest via:
 - 'tp.prefix' = "pr", "se", "*" or "default"
 - 'tp.id_or_tuple' =
    - if 'tp.prefix' = "default" then this value will be empty
    - if 'tp.prefix' != "default":
      - if the value is a vector v then the node has relid v where:
        - if 'tp.prefix' = "pr" or "se" then v describes a processor or a server accordingly
        - if 'tp.prefix' = "*" then
          - v describes node of first node type "xx" where 'G.num_nodes'["xx"] > 0
      - if the value is a size_t v:
        - if prefix = "pr" or "se" then v describes a relid of corresponding node type
        - if prefix = "*" then v describes an absid.
The data needs to be additionally validated when registry has been read and topology_numerics() has executed.
*/
{
    std::string name = "";                  // tp name != ""
    std::size_t num_iterations = 1;         // number of iterations > 0
    std::size_t num_repeats = 1;            // number of repeats > 0
    std::size_t percentage = 100;           // percentage split: > 0 and <= 100
    std::array<TrafficPatternSourceDest, 4> sources_dests{};
};                                          // entries for start-source, end-source, start-dest, end-dest

struct TrafficPatternExpanded
/*
A struct to describe a full tp most of which is set from conversion of an interim tp after validation and
topology_numerics() has executed in validate_traffic_patterns().
*/
{
    std::string name;                       // a validated tp name
    std::size_t num_iterations;             // number of iterations > 0
    std::size_t start_source;               // start-source
    std::size_t end_source;                 // real end-source + 1
    std::size_t start_dest;                 // start-dest
    std::size_t end_dest;                   // real end-dest + 1
    std::array<std::size_t, System::num_node_types + 1> num_faulty_nodes{};
                                            // num. faulty nodes - set in execution()
    std::array<std::size_t, System::num_channel_types + 1> num_faulty_channels{};
                                            // num. faulty channels - set in execution()
};

struct TrafficPattern
/*
A struct to describe all data and parameters associated with the traffic patterns. A global variable 'TP'
will be defined via:
 - TrafficPattern TP;
Any default values are held in 'list_of_config_tags' (apart from the components interim and expanded).
*/
{
    std::vector<std::size_t> batches;             // a list of sizes for each batch (sum = total ]number of
                                                  // tps)
    std::vector<std::uint32_t> seeds;             // a list of seeds for random source/destination generation
                                                  // (System::num_seeds of them)
    std::vector<TrafficPatternInterim> interim;   // the tp descriptions after the config has been read
                                                  // (needed for output writing)
    std::vector<TrafficPatternExpanded> expanded; // the expanded tp descriptions after the interim tps have
                                                  // been validated
};

#endif
