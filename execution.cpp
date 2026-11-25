
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "execution.h"
#include "globals.h"
#include "construction/int_net_construction.h"
#include "data_logging/data_logging.h"
#include "errors/error_writing.h"
#include "faults/impose_faults_from_description.h"
#include "faults/read_faults_from_file.h"
#include "faults/write_faults_to_file.h"
#include "misc/misc_formatting_functions.h"
#include "output/writing_output.h"
#include "statistics/collect_and_compute_stats.h"
#include "structs/structs_routing_stats.h"
#include "traffic/traffic_generators.h"

using std::string;
using std::vector;
using std::size_t;
using std::array;

namespace fs = std::filesystem;

/**
This cpp is concerned with the execution of a run for a given interconnection network, a given
routing algorithm, a (set of) traffic pattern(s) and a (set of) fault-set(s) via:
 - execution().

The structure of execution() is as follows:
 - run routing housekeeping
 - for each traffic batch:
     - for each traffic pattern in that batch:
         - clear all faults in the int net and the fault registration variables
         - impose faults from a fault description
             - possibly reimpose already imposed faults if not regenerating
         - impose faults from a fault file
             - making sure to keep fault files in sync with traffic patterns
         - partition the faults per node-type and channel-type
         - compute the total numbers of faults per node-type and channel-type
             - register these numbers in the current traffic pattern
         - write faults to fault file if requested
         - run the traffic pattern:
             - for however many iterations:
                - for every source-destination pair:
                    - compute the route
                    - check for faults if needs be
                    - log the route - latency
                    - log the loads at nodes/channels
                    - if dealing with synchronous loads:
                        - log the activities at nodes/channels
         - collect data and compute latency statistics for the tp
         - collect data and compute load statistics for the tp
         - if dealing with synchronous loads:
             - collect data and compute overall activity statistics for the tp
             - collect data and compute timed activity statistics for the tp
     - when at end of a batch
         - collect data and compute latency statistics for the batch
         - collect data and compute load statistics for the batch
         - if dealing with synchronous loads:
             - collect data and compute overall activity statistics for the batch
             - collect data and compute timed activity statistics for the batch
 - write the statistics to the output file

Note how faults from fault descriptions are always generated for the first traffic
pattern of a batch.

To do:
 - I need to implement the input of precomputed traffic patterns.
**/

void execution(// supplied data:
               vector<node>&              int_net,
               const int                  l_string_pad,
               vector<ValueAndTriple>&    values_and_triples,
               std::ostringstream&        buffer,
               // other stuff:
               std::ostream&              output)
/*
Executes the traffic pattern runs and builds statistical data

Functions:
 - coding                               from error_writing
 - routing_prelude_ptr                  from <int_net>_routing
 - remove_all_faults                    from read_faults_from_file
 - impose_faults_description            from impose_faults_from_description
 - impose_list_of_faults                from read_faults_from_file
 - read_faults_from_file                from read_faults_from_file
 - partition_faults                     from write_faults_to_file
 - update_fault_numbers                 from write_faults_to_file
 - get_elapsed_time                     from misc_formatting_functions
 - write_faults_to_file                 from write_faults_to_file
 - view_nodes_and_channels              from writing_output
 - traffic_generator_ptr                from traffic_generators
 - routing_alg_ptr                      from <int_net>_routing
 - fault_check_route                    from int_net_construction
 - view_a_route                         from writing_output
 - route_log_basic_latency              from data_logging
 - route_log_basic_loads                from data_logging
 - compute_latency_data                 from collect_and_compute_stats
 - collect_load_data                    from collect_and_compute_stats
 - compute_load_stats                   from collect_and_compute_stats
 - reset_all_loads                      from data_logging
 - collect_activity_data                from collect_and_compute_stats
 - compute_activity_stats               from collect_and_compute_stats
 - reset_all_sync_loads                 from data_logging
 - collect_batch_latency_data           from collect_and_compute_stats
 - collect_batch_load_data              from collect_and_compute_stats
 - collect_batch_activity_data          from collect_and_compute_stats
 - write_the_output_file_header         from writing_output
 - get_output_file_name                 from writing_output
 - warning                              from error_writing
 - write_the_final_output_file          from writing_output
*/
{
    size_t source = 0;
    size_t dest = 0;
    bool started;

    // set up for timers

    std::chrono::high_resolution_clock::time_point start_fault;
    std::chrono::high_resolution_clock::time_point stop_fault;
    string fault_time_string;
    std::chrono::high_resolution_clock::time_point start_overall;
    std::chrono::high_resolution_clock::time_point stop_overall;
    string overall_time_string;

    // set up for maps

    Maps::RoutingAlgFn routing_alg_ptr;             // type of any routing algorithm
    auto routing_alg_it = M.routing_algs.find(RA.name);
    if (routing_alg_it == M.routing_algs.end())
        coding(Errors::BadRoutingAlg);
    else
        routing_alg_ptr = routing_alg_it->second;   // pointer to the routing algorithm function
    Maps::RoutingPreludeFn routing_prelude_ptr;     // type of the routing prelude
    auto routing_prel_it = M.routing_preludes.find(RA.name);
    if (routing_prel_it == M.routing_preludes.end())
        coding(Errors::BadRoutingPrelude);
    else
        routing_prelude_ptr = routing_prel_it->second;// pointer to the routing prelude function

    routing_prelude_ptr();                          // call routing prelude function (housekeeping stuff)

    // set up for routes

    RouteStruct route;                              // the route to be computed
    RouteStruct route_valiant;                      // the phase 2 route if Valiant routing
    route.port_route.reserve(static_cast<size_t>(ceil(std::log2(G.total_nodes))));// reserve route space
    route_valiant.port_route.reserve(static_cast<size_t>(ceil(std::log2(G.total_nodes))));// log_2(|V|)

    // set up for faults

    FaultBatch faults_registered;                   // lists of faults registered for any tp-run
    FaultBatch initial_fault_description;           // initially generated faults from fault description
                                                    //  (because it might need resetting)
    FaultBatch the_faults_read;                     // the faults read from an input fault file
    string the_input_fault_file;
    size_t current_fault_file = 0;                  // match fault file against the tp number

    // set up for traffic patterns

    size_t num_tps = TP.expanded.size();            // number of tps
    size_t num_batches = TP.batches.size() - 1;     // number of batches (note: size is always > 0)
    vector<std::pair<size_t, size_t>> precomputed_tp;// a precomputed tp (if one features)
    size_t index = 0;                               // index in a precomputed tp (if one features)

    // set up for statistics

    vector<RoutingStats> tp_stats(num_tps);         // statistics collection, one per tp
    vector<RoutingStats> batch_stats(num_batches);  // statistics collection, one per batch

    // start simulation

    start_overall = std::chrono::high_resolution_clock::now();// start overall timer

    output << "\nsimulation execution begins ...\n";

    for (size_t batch_no = 0; batch_no < num_batches; ++batch_no)// run a batch of tp runs
    {
        // start a batch

        size_t modified_batch_no = batch_no + 1;    // the batch-number + 1 for output file writing
        if (num_batches > 1)
            output << "\nbatch #" << modified_batch_no << ":\n";// output batch no + 1 for clarity

        for (size_t tp_number = TP.batches[batch_no]; tp_number < TP.batches[batch_no + 1]; ++tp_number)
        {                                                   // for each tp in the batch
            size_t modified_tp_number = tp_number + 1;// the tp-number + 1 for output file writing
            output << "\ntraffic pattern run " << tp_number + 1 - TP.batches[batch_no] << " out of "
                        << TP.batches[batch_no + 1] - TP.batches[batch_no] << " ...\n";

            // start fault imposition

            start_fault = std::chrono::high_resolution_clock::now();// start timing of fault-imposition phase

            remove_all_faults(int_net);             // clear faults and faults registered
            faults_registered = FaultBatch{};       // initialize faults registered

            // impose faults from a fault description

            if (!FAU.fault_description.empty())     // if there the are some faults described then impose them
            {
                if (tp_number == TP.batches[batch_no])// if the first tp of batch then impose faults description
                {
                    impose_faults_description(int_net, faults_registered, output);
                    if (!FAU.regenerate)            // if we keep initial faults generated from fault description
                    {                               //  then remember them
                        initial_fault_description.nodes = faults_registered.nodes;
                        initial_fault_description.channels = faults_registered.channels;
                        initial_fault_description.ports = faults_registered.ports;
                    }
                }
                else
                {
                    if (FAU.regenerate)             // if we need to generate new faults then do so
                        impose_faults_description(int_net, faults_registered, output);
                    else                            // otherwise reimpose faults from initial generation
                    {
                        impose_list_of_faults(int_net, initial_fault_description, faults_registered);
                        output << "\nkeeping initial faults from fault descriptions\n";
                    }
                }
            }

            // impose faults from a fault file

            if (!FAU.input_fault_files.empty())     // if there is a fault file to be read
            {
                the_input_fault_file = FAU.input_fault_files[current_fault_file];// get fault file
                current_fault_file++;               // reset as next file in the list
                if (current_fault_file == FAU.input_fault_files.size())
                    current_fault_file = 0;
                read_faults_from_file(int_net, the_input_fault_file, the_faults_read, output);// read
                impose_list_of_faults(int_net, the_faults_read, faults_registered);// impose
            }

            // all faults have now been imposed and so partition them per node- or channel-type,
            // register the various numbers of faults in the tp struct and write faults to file

            array<vector<size_t>, System::num_node_types>    array_of_faults_nodes;
            array<vector<size_t>, System::num_channel_types> array_of_faults_channels;
            array<vector<size_t>, System::num_channel_types> array_of_faults_ports;

            partition_faults(int_net, faults_registered, array_of_faults_nodes, array_of_faults_channels,
                             array_of_faults_ports, output);
            update_fault_numbers(tp_number, array_of_faults_nodes, array_of_faults_channels);

            if (num_tps == 1)                       // if 1 tp then tp number does not feature in output
                modified_tp_number = 0;             //  file name (signalled by 'modified_tp_number' = 0)

            stop_fault = std::chrono::high_resolution_clock::now();// stop timing of fault-impose phase
            fault_time_string = get_elapsed_time(start_fault, stop_fault);

            write_faults_to_file(int_net, modified_tp_number, the_input_fault_file, fault_time_string,
                    array_of_faults_nodes, array_of_faults_channels, array_of_faults_ports, output);
                                                    // function handles whether to sort and write
            // run the traffic pattern

            output << "\nstarting computations of routes ...\n";

            size_t failed_routes = 0;
            size_t successful_routes = 0;
            size_t times_tried = 0;
            vector<size_t> cumulative_hop_lengths;

            Maps::TrafficPatternFn traffic_generator_ptr;   // type of traffic generator
            auto traffic_it = M.traffic_patterns.find(TP.expanded[tp_number].name);
            if (traffic_it == M.traffic_patterns.end())
                coding(Errors::BadTPName);
            else
                traffic_generator_ptr = traffic_it->second;

            for (size_t c = 0; c < TP.expanded[tp_number].num_iterations; ++c)
            {
                started = false;                    // inform traffic gen. that run has yet to start
                while (traffic_generator_ptr(tp_number, started, source, dest, precomputed_tp,
                                             index) == true)
                {
                    route.source = source;          // set the route source, for each route
                    route.error_flag = {Errors::Good, 0};// assume healthy route
                    route.port_route.clear();       // clear the vector of ports, for each route
                    route.times_tried = 0;          // reset the number of routes tried
                    if (RA.valiant)
                    {
                        size_t random_terminal = guess_a_possible_source("*");
                        routing_alg_ptr(int_net, source, random_terminal, route);// run phase 1
                        route_valiant.source = random_terminal;
                        route_valiant.error_flag = {Errors::Good, 0};
                        route_valiant.port_route.clear();
                        routing_alg_ptr(int_net, random_terminal, dest, route_valiant);// run phase 2
                        route.port_route.insert(route.port_route.end(), route_valiant.port_route.begin(),
                                                route_valiant.port_route.end());
                    }
                    else
                        routing_alg_ptr(int_net, source, dest, route);// run the routing algorithm
                    if ((FAU.fault_check == FaultCheck::post)// if the route should be fault-checked ...
                        || (FAU.fault_check == FaultCheck::interim_en_route_and_post)
                        || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
                        route.error_flag = fault_check_route(int_net, source, dest, route);
                    if ((ENV.view_routes == ViewRoutes::all) || ((ENV.view_routes == ViewRoutes::bad)
                            && (route.error_flag.code != Errors::Good)))// if a route is to be viewed ...
                        view_a_route(int_net, source, dest, route, output);
                    route_log_basic_latency(route, failed_routes, successful_routes,// compute latency data
                                            cumulative_hop_lengths, times_tried);   //  data but per pat
                    route_log_basic_loads(route, int_net);// log the load data per pat (and possibly
                }                                         //  act. load data)
            }

            if (ENV.view_int_net_with_faults)       // view the int. net. with faults if required
            {
                view_nodes_and_channels(int_net, output);
                output << "\n";
            }

            output << "... traffic pattern run " << tp_number + 1 - TP.batches[batch_no] << " out of "
                        << TP.batches[batch_no + 1] - TP.batches[batch_no] << " finished\n";

            output << "\ncollecting data and computing statistics from run ...\n";

            compute_latency_data(failed_routes, successful_routes, cumulative_hop_lengths, times_tried,
                                 tp_stats[tp_number]);
            collect_load_data(int_net, tp_stats[tp_number]);
            compute_load_stats(tp_stats[tp_number]);
            reset_all_loads(int_net);
            if (ENV.sync)
            {
                collect_activity_data(int_net, tp_number, tp_stats[tp_number]);
                compute_activity_stats(tp_stats[tp_number]);
                reset_all_sync_loads(int_net);
            }

            output << "... statistics from run computed\n";
        }

        output << "\ncollecting data and computing statistics from batch ...\n";

        size_t batch_failed_routes = 0;
        size_t batch_successful_routes = 0;
        size_t batch_times_tried = 0;
        vector<size_t> batch_cumulative_hop_lengths;
        batch_cumulative_hop_lengths.reserve(static_cast<size_t>
                                            (ceil(std::log2(G.total_nodes))));// guess at log_2(|V|)

        collect_batch_latency_data(TP.batches[batch_no], TP.batches[batch_no + 1], tp_stats,
                                   batch_failed_routes, batch_successful_routes,
                                   batch_cumulative_hop_lengths, batch_times_tried);
        compute_latency_data(batch_failed_routes, batch_successful_routes,
                             batch_cumulative_hop_lengths, batch_times_tried, batch_stats[batch_no]);
        collect_batch_load_data(TP.batches[batch_no], TP.batches[batch_no + 1], tp_stats,
                                batch_stats[batch_no]);
        compute_load_stats(batch_stats[batch_no]);
        if (ENV.sync)
        {
            collect_batch_activity_data(TP.batches[batch_no], TP.batches[batch_no + 1], tp_stats,
                                        batch_stats[batch_no]);
            compute_activity_stats(batch_stats[batch_no]);
        }

        output << "... statistics from batch computed\n";

        output << "\nall traffic pattern runs in batch " << modified_batch_no << " are now finished\n";
    }

    output << "\nthe overall traffic pattern execution is now finished\n";

    stop_overall = std::chrono::high_resolution_clock::now();// end overall timing
    overall_time_string = get_elapsed_time(start_overall, stop_overall);

    string output_file_name = get_output_file_name(values_and_triples);
    fs::path dir = fs::current_path() / fs::path("ACTUAL_NETWORKS") / IN.name / "output_files";
    if (!fs::exists(dir))
        fs::create_directories(dir);
    fs::path file_path = dir / output_file_name;

    std::ofstream output_file(file_path);
    if (!output_file)
    {
        warning(Errors::CantOpenFile, output_file_name);
        return;
    }

    write_the_final_output_file(int_net, values_and_triples, l_string_pad, overall_time_string, batch_stats,
                                tp_stats, buffer, output_file);
    output_file.close();
}
