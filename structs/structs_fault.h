
#ifndef STRUCTS_FAULT_H
#define STRUCTS_FAULT_H

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

/**
***********************
Fault enums and structs
***********************
**/

enum class FaultCheck : std::size_t
/*
An enum to detail the mode of fault-checking:
 - post                         after the route has been computed and returned, fault-check
 - interim_en_route             build route noting faulty components so no need to fault-check
 - interim_en_route_and_post    build route noting faulty components but also post-check (for debugging)
 - interin_faulty               build route without noting faulty components and fault-check after
                                route built
 - interin_faulty_and_post      build route without noting faulty components and internally fault-check
                                after route built and also post-check (for debugging)
 - none                         no fault checking.
*/
{
    post, interim_en_route, interim_en_route_and_post, interim_at_end, interim_at_end_and_post, none
};

struct FaultDescription
/*
A struct to describe a full fault description. Note the initial values.
*/
{
    std::string    name = "";              // in {"node", "channel", "link"}
    double         percentage = 0;         // 0 <= percentage < 100
    std::size_t    raw_count = 0;          // 0 <= raw_count < max possible depending on 'name'
    std::string    source_node_type = "*"; // in {"pr", "se", "sw", "os", "*"} where "*" means "don't care"
    std::string    dest_node_type = "*";   // in {"pr", "se", "sw", "os", "*"}
    std::size_t    number_of_faults = 0;   // the number of faults to be set (calculated when faults are set)
};

struct Fault
/*
A struct to describe all data and parameters associated with any faults. A global variable 'FAU' will be
defined via:
 - Fault FAU;
Any default values are held in 'config_tags' (apart from the components 'list_of_saved_fault_files' and
'fault_description').
*/
{
    FaultCheck                       fault_check;
                                     // how we want to check for faults from {"post", "interim_en_route",
                                     //  "interim_at_end", "none"}
    std::vector<std::string>         input_fault_files;
                                     // a list of files from which to read some faults
    std::string                      output_fault_file;
                                     // "false" = no output; "true" = guess name; otherwise take as name
    std::vector<std::uint32_t>       seeds;
                                     // list of seeds for random fault generation (System::num_seeds of them)
    std::size_t                      limit_guesses;
                                     // number of allowed bad guesses when imposing random faults (>= 0)
    bool                             show_generation;
                                     // show fault guessing process as faults generated (false = no show)
    bool                             sort_faults;
                                     // whether to sort faults before writing to file (if large number of
                                     //  faults then don't): true = yes, false = no
    bool                             regenerate;
                                     // true = regenerate fault description faults for every traffic
                                     //  pattern; false = re-use the initial fault description faults
    std::vector<std::string>         list_of_saved_fault_files;
                                     // the names of fault files saved for each pattern
    std::string                      suffix;
                                     // a suffix (no spaces) for output file name hinting at fault scenario
    std::vector<FaultDescription>    fault_description;
                                     // descriptions of faults arising from config
};

struct FaultBatch
/*
A description of:
 - a batch of faulty nodes as a vector of nodes 'nodes' (absid's)
 - a batch of faulty channels as
    - a vector of nodes 'channels'
    - a vector of ports 'ports'
   so that 'channels' and 'ports' have identical lengths.
*/
{
    std::vector<std::size_t>    nodes;
    std::vector<std::size_t>    channels;
    std::vector<std::size_t>    ports;
};

#endif
