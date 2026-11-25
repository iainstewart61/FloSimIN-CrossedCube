
#include <cstdint>
#include <map>

#include "globals.h"

/**
This ccp contains all of the globals used within the program. I use globals as these variables are set up
very early in an execution and never change value afterwards (they are essentially constant).
**/

/*
System globals G. Those not defined in "structs_global" are defined via initialize_system_globals()
(below) which is the first function called in main().
*/

System G;

void initialize_system_globals()
/*
This function initializes basic system information that cannot be initialized in "structs_system". It must
be the first function called in main().
*/
{
    // there must be 'System::num_term_types' entries
    G.node_types[System::pr] = NodeType{"processor", "pr", true};
    G.node_types[System::se] = NodeType{"server", "se", true};

    // there must be 'System::num_switch_types' entries
    G.node_types[System::sw] = NodeType{"switch", "sw", false};
    G.node_types[System::os] = NodeType{"optical switch", "os", false};

    for (size_t i = 0; i < System::num_node_types; ++i) // there must be 'System::num_channel_types' entries
    {
        for (size_t j = 0; j < System::num_node_types; ++j)
        {
            size_t index = nn_to_c(i, j);
            G.channel_types[index].name = G.node_types[i].name + "-" + G.node_types[j].name;
            G.channel_types[index].short_name = G.node_types[i].short_name + G.node_types[j].short_name;
        }                                                                       // so 4-character short names
    }

    G.global_size_t = 0;
}

/*
Globals specific to the interconnection network and if marked [CONFIG] read from config under <INTERCONNECTION
NETWORK>. The components are:
 - string name                  name of the network                                             [CONFIG core]
 - vector<string> param_names   names of key net. params (> 0 with first always the dimension   [CONFIG core]
                                and usually called "n") read from config and needs to match
                                exactly with param. names read from registry
 - vector<size_t> param_values  key parameters for the network (dimension always the first and  [CONFIG core]
                                all must be > 0)
 - vector<uint32_t> seeds       a list of seeds for random generation for, e.g., dimension      [CONFIG]
                                orders (System::num_seeds of them)
Note that 'param_names' and 'param_values' are obtained by reading a vector of names/values from the config
file that is then split.
*/

InterconnectionNetwork IN;

/*
Globals specific to the routing algorithm and if marked [CONFIG] read from config under <ROUTING ALGORITHM>.
The components are:
 - string name               the routing algorithm (a non-empty string)                         [CONFIG core]
 - RoutingDor dor            the type of dimension order                                        [CONFIG]
 - RoutingTiebreak tiebreak  the tiebreak protocol                                              [CONFIG]
 - RoutingMode mode          the routing execution mode                                         [CONFIG]
 - int nondet_repeat         how many non-deterministic runs we wish to attempt (> 0)           [CONFIG]
 - int increm_bound          how many incremental turns we wish to allow (-1 = unlimited)       [CONFIG]
 - bool valiant              whether to run Valiant's algorithm                                 [CONFIG]
 - vector<uint32_t> seeds    a list of seeds for random generation in routing algorithms        [CONFIG]
                            (System::num_seeds of them)
*/

RoutingAlgorithm RA;

/*
Globals specific to the traffic pattern and if marked [CONFIG] read from config under <TRAFFIC PATTERN>.
The components are:
 - vector<size_t> batches                   a list of sizes for each batch (sum = total         [CONFIG]
                                            number of tps)
 - vector<uint32_t> seeds                   a list of seeds for random source/destination       [CONFIG]
                                            generation (System::num_seeds of them)
 - vector<TrafficPatternInterim> interim    the tp descriptions after the config has been read  [CONFIG]
                                            (needed for output writing)
 - vector<TrafficPatternExpanded> expanded  the expanded tp descriptions after the interim tps
                                            have been validated
Note that 'expanded' is built in validate_traffic_patterns() after the interconnection network has been built
and topology_numerics() has been executed.
*/

TrafficPattern TP;

/*
Globals specific to the environment and if marked [CONFIG] read from config under <ENVIRONMENT>. The
components are:
 - bool sync                      denotes whether synchronous data is to be collected            [CONFIG]
 - size_t sync_worm               the length (> 0) of the worm in synchronous loads              [CONFIG]
 - bool suppress_tps              true = don't write individual tp data to output file           [CONFIG]
 - bool use_used_mode             used mode = nodes/channels are used or unused                  [CONFIG]
 - ViewRoutes view_routes         "all" = display all, "bad" = only bad routes, "none" = none    [CONFIG]
 - bool view_int_net              true = display interconnection network, false = don't          [CONFIG]
 - bool view_int_net_with_faults  true = display int. net. with faults, false = don't            [CONFIG]
 - bool view_loads                true = if viewing int net, view loads as well                  [CONFIG]
 - ViewGlobals view_globals       view globals on screen/in file: "none" = no; "all" = all       [CONFIG]
                                  globals; "nondef" = non-defaults only
 - bool rewrite_registry;         whether to rewrite the registry file                           [CONFIG]
*/

Environment ENV;

/*
Globals specific to faults and if marked [CONFIG] read from config under <FAULT>. The
components are:
 - FaultCheck fault_check                      how we want to check for faults from {"post",    [CONFIG]
                                               "interim_en_route", "interim_at_end", "none"}
 - vector<string> input_fault_files            a list of files from which to read some faults   [CONFIG]
 - string output_fault_file                    "false" = no output; "true" = guess name;        [CONFIG]
                                               otherwise take as name
 - vector<uint32_t> seeds                      a list of seeds for random fault generation      [CONFIG]
                                               (System::num_seeds of them)
 - int limit_guesses                           number of allowed bad guesses when imposing      [CONFIG]
                                               random faults (>= 0)
 - bool show_generation_process                show fault guessing process as faults generated  [CONFIG]
                                               (false = no show)
 - bool sort_faults                            whether to sort faults before writing to file    [CONFIG]
                                               (if large number of faults then don't): true =
                                               yes, false = no
 - bool regenerate                             true = regenerate fault description faults for   [CONFIG]
                                               every traffic pattern; false = re-use the
                                               initial fault description faults
 - vector<string> list_of_saved_files          the names of fault files saved for each pattern
 - string suffix                               a suffix (no spaces) for output file name        [CONFIG]
                                               hinting at fault scenario
 - vector<FaultDescription> fault_description  descriptions of faults arising from config       [CONFIG]
Note that 'list_of_saved_files' is built automatically depending upon 'output_fault_file'. The
descriptions in 'fault_description' are ratified in validate_fault_descriptions().
*/

Fault FAU;

/*
Globals specific to latency and if marked [CONFIG] read from config under <LATENCY> (see: A. Erickson,
A.E. Kiasari, J. Navaridas and I.A. Stewart, An optimal single-path routing algorithm in the datacenter
network DPillar, IEEE Trans. Parallel Distributed Syst. 28(3) 689-703 (2017) for latency classification).
The components are:
 - bool refined_latency                    if true then 'per_hop_latency' = sum of others       [CONFIG]
 - vector<size_t> stack_latency            stack latency (in microsec) of 4 node types          [CONFIG]
 - vector<size_t> propagation_latency      propagation latency (in microsec) of (pr, se, sw,    [CONFIG]
                                           os)[
 - vector<size_t> data_transfer_latency    data transfer latency (in microsec) of (pr, se, sw,  [CONFIG]
                                           os)
 - vector<size_t> average_routing_latency  average routing latency (in microsec) of (pr, se,    [CONFIG]
                                           sw, os)
 - vector<size_t> per_hop_latency          total per hop latency (in microsec) of (pr, se, sw,  [CONFIG]
                                           os)
 - size_t os_recon_latency                 optical switch reconfiguration latency (in           [CONFIG]
                                           microsec)
*/

Latency LAT;

void latency_updating()
/*
This function sets refined_latency as:
 - stack_latency + propagation_latency + data_transfer_latency + average_routing_latency
as per the above paper. It should be called after config has been read and processed via
globals_housekeeping().
*/
{
    if (LAT.refined_latency == true)
    {
        for (size_t i = 0; i < LAT.per_hop_latency.size(); i++)
        {
            LAT.per_hop_latency[i] = LAT.stack_latency[i] + LAT.propagation_latency[i]
                                     + LAT.data_transfer_latency[i] + LAT.average_routing_latency[i];
        }
    }
}

/*
Globals specific to bandwidth and if marked [CONFIG] read from config under <BANDWIDTH>. The components are:
 - vector<size_t> port_bandwidth  chan. bandw. (in bits/sec) of 16 different chan. types        [CONFIG]
*/

Bandwidth BAN;

/*
Globals specific to packets and if marked [CONFIG] read from config under <PACKET>. The components are:
 - size_t packet_size  size of a packet (in bits)                                               [CONFIG]
 - size_t header_size  size of a packet header (in bits)                                        [CONFIG]
*/

Packet PKT;

/*
Globals specific to notes and if marked [CONFIG] read from config under <NOTES>. The components are:
 - string added_note   an added note                                                            [CONFIG]
*/

Notes NOT;

/*
Globals specific to the interconnection network and set via read_int_net_data_from_registry() (no defaults as
read from registry). The components are:
 - RegistryNetMachine net_machine  the interconnection network machine
 - RegistryNetType net_type        the interconnection network type
 - RegistryNetBuild net_build      whether the interconnection network has been symmetrically built
 - string registry_note            a note in the registry about the network
*/

RegistryGlobals RG;

/*
Globals specific to the routing algorithm and set in <routing_algorithm>_prelude(). The components are:
 - vector<size_t> dim_order                 a specific dimension order
*/

RoutingAlgorithmPrelude RAP;

/*
Random number generators.
*/

RandomGenerator RNG_fault{};    // random number gen. for fault generation
RandomGenerator RNG_tp{};       // random number gen. for source/destination generation in traffic patterns
RandomGenerator RNG_misc{};     // random number gen. for, e.g., dimension order generation
RandomGenerator RNG_routing{};  // random number gen. for random generation in routing algorithms

/*
The map variables to associate functions with an interconnection network IN.name:
 - int_net_globals
 - int_net_build
 - routing_algs
 - routing_preludes
 - traffic_patterns
 - alg_to_numeric
 - alg_from_numeric
*/

Maps M;

void globals_housekeeping()
/*
Any updates after reading and validating globals data.
*/
{
    latency_updating();
}

