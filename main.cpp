
#include <iostream>
#include <vector>
#include <string>

#include "execution.h"
#include "globals.h"
#include "maps.h"
#include "construction/int_net_construction.h"
#include "input/data_structures_for_config.h"
#include "input/read_config_file.h"
#include "input/read_registry_file.h"
#include "input/validation.h"
#include "input/writing_config_reg.h"
#include "output/writing_output.h"

using namespace std;

int main()
{
    std::ostream&    output = cout;

    /*************************************************
    SET EVERYTHING UP, READ ALL OF THE INPUT DATA FROM
    CONFIG AND REGISTRY AND THEN VALIDATE THIS DATA
    *************************************************/

    // initialize the system globals

    initialize_system_globals();// from "globals"

    // set up the config/registry data structures and read in the config data

    string                    config_file_name = "config.txt";
    vector<Tag>               config_tags;
    vector<ValueAndTriple>    values_and_triples;

    read_config_file(config_file_name, config_tags, values_and_triples, output);// from "read_config_file"

    // read in the registry data

    string         registry_file_name = "registry.txt";
    TheRegistry    the_registry;
    string         pre_registry;
    string         post_registry;

    read_int_net_data_from_registry(registry_file_name, values_and_triples, the_registry, pre_registry,
                                    post_registry, output);// from "read_registry_file"

    // validate the core input and registry data

    validate_core_int_net_data(the_registry, output);// from "validation"

    // initialize the various maps

    setup_all_maps(M.int_net_globals, M.int_net_build, M.routing_algs, M.routing_preludes,
                   M.traffic_patterns, M.alg_to_numeric, M.alg_from_numeric);// from "maps"

    // set and validate the globals for the chosen interconnection network

    Maps::IntNetGlobalsFn globals_ptr;
    globals_ptr = M.int_net_globals[IN.name];
    globals_ptr();                  // calls <int_net>_set_globals()
    validate_int_net_globals();     // from "validation"

    // compute the numeric topology data

    topology_numerics(output);      // from "int_net_construction"

    // globals housekeeping

    globals_housekeeping();         // from "globals"

    // additional traffic pattern and fault validation

    validate_traffic_patterns(config_tags);// from "validation"
    validate_fault_descriptions();  // from "validation"
    if (ENV.rewrite_registry)
        rewrite_the_registry(registry_file_name, values_and_triples, pre_registry, post_registry, the_registry);
                                                                                            // from "validation"
    // define random number generators (using any global seeds if needs be)

    RNG_misc = RandomGenerator(IN.seeds);   // from "random_gen"
    IN.seeds = RNG_misc.getUsedSeeds();
    RNG_routing = RandomGenerator(RA.seeds);// from "random_gen"
    RA.seeds = RNG_routing.getUsedSeeds();
    RNG_tp = RandomGenerator(TP.seeds);     // from "random_gen"
    TP.seeds = RNG_tp.getUsedSeeds();
    RNG_fault = RandomGenerator(FAU.seeds); // from "random_gen"
    FAU.seeds = RNG_fault.getUsedSeeds();

    // view globals data and possibly view systems globals and complete registry

    int left_pad = 42;              // defines formatting in output files

    std::ostringstream buffer;      // prepare output from write_globals() in 'buffer'
    write_globals(config_tags, left_pad, the_registry, values_and_triples, buffer);
    output << buffer.str();         // from "writing_config_reg"

    /****************************
    BUILD INTERCONNECTION NETWORK
    ****************************/

    output << "\nbuilding the interconnection network ...\n";

    vector<node> int_net;
    Maps::IntNetBuildFn build_ptr;
    build_ptr = M.int_net_build[IN.name];// builds using <int_net>_build()

    // interconnection network construction

    prelude_to_int_net_build(int_net);  // from "int_net_construction"
    build_ptr(int_net);                 // builds using <int_net>_build()
    postlude_to_int_net_build();        // from "int_net_construction"

    output << "... interconnection network built\n";

    /*************************************************
    VIEW INTERCONNECTION NETWORK DETAILS (IF REQUIRED)
    *************************************************/

    // formatting for writing interconnection network content

    compute_pad_values_for_writing();               // from "writing_output"

    // view topology numerics and the (fault-free) interconnection network topology (if required)

    if (ENV.view_int_net)
    {
        view_int_net_topology_numerics(left_pad, output);// from "writing_output"
        view_nodes_and_channels(int_net, output);// from "writing_output"
    }

    /***************************
    EXECUTE ALL TRAFFIC PATTERNS
    ***************************/

    execution(int_net, left_pad, values_and_triples, buffer, output);
}


