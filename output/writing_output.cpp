
#include <chrono>
#include <variant>

#include "writing_output.h"
#include "../misc/misc_formatting_functions.h"
#include "../globals.h"
#include "../input/data_structures_for_config.h"
#include "../errors/error_writing.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp contains functions concerning viewing core data:
 - view_nodes_and_channels()
 - view_int_net_topology_numerics()
a function to view routes
 - view_a_route()
and numerous functions to write data to the output file. There is also:
 - compute_pad_values_for_writing()
which needs to be called immediately after the interconnection network has been built as it works out
all of the formatting parameters.

Functions for initializing spacing;
 - compute_pad_values_for_writing()
Functions for writing individual nodes, channels, routes and lines:
 - partial_node()
 - write_a_node_entry()
 - write_a_channel_entry()
 - view_a_route()
 - write_line()
Overall view functions:
 - view_nodes_and_channels()
 - view_int_net_topology_numerics()
Functions concerning the output file name:
 - get_output_file_name()
File writing functions:
 - write_the_output_file_header
 - write_latency_stats()
 - stat_to_string()
 - assign_stat()
 - write_load_act_stats()
 - write_node_load_stats()
 - write_channel_load_stats()
 - write_node_activity_stats
 - write_channel_activity_stats
 - write_node_timed_activity_stats()
 - write_channel_timed_activity_stats()
Write output file
 - write_the_final_output_file()
**/

/**
**********************************
Functions for initializing spacing
**********************************
**/

void compute_pad_values_for_writing()
/*
For each node-type, set up:
 - 'pad_abs'
 - 'pad_rel'
 - 'pad_bit'
pad values for writing output. Function to be called immediately after the interconnection network
has been built.

Functions:
 - get_fixed_width          from misc_formatting_functions
*/
{
    for (auto& nt : G.node_types)
    {
        if (nt.number > 0)
        {
            nt.pad_abs = get_fixed_width(nt.last - 1);
            nt.pad_rel = get_fixed_width(nt.number - 1);
            nt.pad_bit = 0;
            for (size_t i = 0; i < nt.alg_rep; ++i)
            {
                if (nt.alg_rep_range[i] > nt.pad_bit)
                    nt.pad_bit = nt.alg_rep_range[i];
            }
            nt.pad_bit = get_fixed_width(nt.pad_bit - 1);
        }
    }
}

/**
******************************************************************
Functions for writing individual nodes, channels, routes and lines
******************************************************************
**/

string partial_node(// supplied data:
                    const vector<node>&    int_net,
                    const size_t           abs_id,
                    const bool             reverse_it)
/*
Create  partial description of a node:
 - pr16 (123) [1,0,0,0,0]
If 'reverse_it' is true then the partial description is reversed as:
 - [1,0,0,0,0] (123) pr16

Functions:
 - truncate_or_pad          from misc_formatting_functions
 - vector_to_a_string       from misc_formatting_functions
*/
{
    size_t n_type = int_net[abs_id].node_get_type();
    size_t rel_id = abs_id - G.node_types[n_type].first;
    string n_type_str = G.node_types[n_type].short_name;
    string rel_id_str = truncate_or_pad(rel_id, static_cast<int>(G.node_types[n_type].pad_rel));
    string abs_id_str = truncate_or_pad(abs_id, -1 * static_cast<int>(G.node_types[n_type].pad_abs));
    string n_algrep_str = vector_to_a_string(int_net[abs_id].node_get_alg(), G.node_types[n_type].pad_bit);
    string node_entry;
    if (reverse_it == false)
        node_entry = n_type_str + rel_id_str + " (" + abs_id_str + ") " + n_algrep_str;
    else
        node_entry = n_algrep_str + " (" + abs_id_str + ") " + n_type_str + rel_id_str;
    return node_entry;
}

string write_a_node_entry(// supplied data:
                          const vector<node>&    int_net,
                          const size_t           abs_id)
/*
Prepare a string that fully describes a node for writing to file or display, e.g.,
 - pr16 (123) [1,0,0,0,0] f pat:load/actload 55/[0,34,23,11,9] batch:load/actload 111/[0,45,55,22,9,5,4]
We assume that 'abs_id' is valid.

Functions:
 - partial_node                 from here
 - make_dense_vector            from misc_formatting_functions
 - vector_to_a_string           from misc_formatting_functions
*/
{
    string node_entry = partial_node(int_net, abs_id, false);
    string n_fault_str;
    if (int_net[abs_id].node_get_health() == System::faulty)
        n_fault_str = "f";
    else
        n_fault_str = "-";
    node_entry += " " + n_fault_str;
    if (ENV.view_loads == true)
    {
        string n_pat_load_str = std::to_string(int_net[abs_id].node_get_load());
        vector<size_t> n_pat_actload;
        make_dense_vector(int_net[abs_id].node_get_act_load(), n_pat_actload);
        string n_pat_actload_str = vector_to_a_string(n_pat_actload, 0);
        node_entry += " load/actload " + n_pat_load_str + "/" + n_pat_actload_str;
    }
    return node_entry;
}

string write_a_channel_entry(// supplied data:
                             const vector<node>&    int_net,
                             const size_t           abs_id_src,
                             const size_t           port_src)
/*
Prepare a string that fully describes a channel for writing to file or display, e.g.,
 - pr16 (123) [1,0,0,0,0] f [0,1,1,0,0] (119) pr12 load/syncload 5/[0,4,5] batch:load/syncload 5/[0,4,5]
We assume that 'abs_id_src' and 'port_scr' are valid.

Functions:
 - partial_node                 from here
 - make_dense_vector            from misc_formatting_functions
 - vector_to_a_string           from misc_formatting_functions
*/
{
    string channel_entry = partial_node(int_net, abs_id_src, false);
    string c_fault_str;
    if (int_net[abs_id_src].node_get_port_health(System::out, port_src) == System::faulty)
        c_fault_str = "f";
    else
        c_fault_str = "-";
    channel_entry += " " + c_fault_str + " ";
    size_t abs_id_tgt =int_net[abs_id_src].node_get_port_node(System::out, port_src);
    channel_entry += partial_node(int_net, abs_id_tgt, true);
    string port_src_str = std::to_string(port_src);
    channel_entry += " port : " + port_src_str;

    string c_pat_load_str = std::to_string(int_net[abs_id_src].node_get_port_load(port_src));
    vector<size_t> c_pat_syncload;
    make_dense_vector(int_net[abs_id_src].node_get_port_act_load(port_src), c_pat_syncload);
    string c_pat_syncload_str = vector_to_a_string(c_pat_syncload, 0);

    if (ENV.view_loads == true)
        channel_entry += " load/syncload " + c_pat_load_str + "/" + c_pat_syncload_str;
    return channel_entry;
}

void view_a_route(// supplied data:
                  const vector<node>&    int_net,
                  const size_t           source,
                  const size_t           dest,
                  const RouteStruct&     route,
                  // other stuff:
                  std::ostream&          output)
/*
View a route:
source : pr48 (103) [1,1,0,0,0,0]
dest   : pr3  ( 58) [0,0,0,0,1,1]
  pr48 (103) [1,1,0,0,0,0] -1-> pr50 (105) [1,1,0,0,1,0] -5-> pr18 ( 73) [0,1,0,0,1,0]
  -4-> pr2  ( 57) [0,0,0,0,1,0] -1-> pr3  ( 58) [0,0,0,0,1,1]

Functions:
 - oartial_node         from here
*/
{
    string port_str;
    string node_str;
    string source_str = partial_node(int_net, source, false);
    output <<  "source : " << source_str << "\n";
    output <<  "dest   : " << partial_node(int_net, dest, false) << "\n";
    size_t length = route.port_route.size();
    output << source_str;
    size_t current_id = source;
    for (size_t i = 0; i < length; ++i)
    {
        port_str = "-" + std::to_string(route.port_route[i]) + "->";
        current_id = int_net[current_id].node_get_port_node(System::out, route.port_route[i]);
        node_str = partial_node(int_net, current_id, false);
        output << " " << port_str << " " << node_str;
    }
    if (route.error_flag.code == Errors::FaultySource)
        output << " fault: faulty source";
    else if (route.error_flag.code == Errors::BadDestination)
        output << " fault: bad destination";
    else if (route.error_flag.code == Errors::BadError)
        output << " fault: bad error flag";
    else if (route.error_flag.code == Errors::FaultyNode)
        output << " fault: faulty node #" << route.error_flag.location;
    else if (route.error_flag.code == Errors::FaultyChannel)
        output << " fault: faulty channel #" << route.error_flag.location;
    output << "\n";
}

void write_line(// supplied data:
                const LabelValue&    lv,
                const size_t         label_width,
                // other stuff:
                std::ostream&        output)
/*
Writes a basic line in a file.
*/
{
    output << std::left << std::setw(label_width) << lv.label << " : " << lv.value << "\n";
}

/**
**********************
Overall view functions
**********************
**/

void view_nodes_and_channels(// supplied data:
                             const vector<node>&    int_net,
                             // other stuff:
                             std::ostream&          output)
/*
Print out a full node description of the interconnection network together with which nodes and channels
are faulty. If 'ENV.view_loads' = true then loads for the pattern and run are output. This function is
normally only used when debugging. We assume that 'nt.alg_rep_range' is such that all entries are at
least 2.

Functions:
 - write_a_node_entry               from here
 - write_a_channel_entry            from here
*/
{
    output << "\nNODES\n";

    for (auto& nt : G.node_types)
    {
        if (nt.number > 0)
        {
            for (size_t abs_id = nt.first; abs_id < nt.last; ++abs_id)
            {
                output << write_a_node_entry(int_net, abs_id) << "\n";
            }
        }
    }

    output << "\nCHANNELS\n";

    for (auto& nt : G.node_types)
    {
        if (nt.number > 0)
        {
            for (size_t abs_id = nt.first; abs_id < nt.last; ++abs_id)
            {
                size_t num_ports = int_net[abs_id].node_get_num_ports();
                for (size_t j = 0; j < num_ports; ++j)
                {
                    if (int_net[abs_id].node_get_port_mode(System::out, j) == System::used)
                    {
                        output << write_a_channel_entry(int_net, abs_id, j) << "\n";
                    }
                }
            }
        }
    }
}

void view_int_net_topology_numerics(// supplied data:
                                    const int l_string_pad,
                                    // other stuff:
                                    std::ostream &output)
/*
View the core numeric data for the interconnection network. The supplied data can be overwritten in
general if required.

Functions:
 - write_line                   from here
 - vector_to_a_string           from misc_formatting_functions
*/
{
    string r_string;
    string l_string;
    output << "\n*** TOPOLOGY NUMERICS ***\n\n";
    r_string = IN.name + "[";
    size_t num_param_names = IN.param_names.size();
    for(size_t i = 0; i < num_param_names; ++i)
    {
        r_string += IN.param_names[i] + "=" + std::to_string(IN.param_values[i]);
        if (i < num_param_names - 1)
            r_string += ",";
    }
    r_string += "]";
    write_line({"interconnection network", r_string}, l_string_pad, output);
    output << "\n";
    write_line({" total number of terminals", std::to_string(G.num_terms)}, l_string_pad, output);
    write_line({" total number of non-terminals", std::to_string(G.num_switches)}, l_string_pad, output);
    write_line({" total number of nodes", std::to_string(G.total_nodes)}, l_string_pad, output);
    write_line({" total number of channels", std::to_string(G.total_channels)}, l_string_pad, output);

    bool written_non_terminal_header = false;
    output << " terminal types:\n";
    for (size_t i = 0; i < G.num_term_types; ++i)
    {
        if (G.node_types[i].number > 0)
        {
            l_string = "";
            if (i >= System::num_term_types)
            {
                if (!written_non_terminal_header)
                {
                    output << " non-terminal types:\n";
                    written_non_terminal_header = true;
                }
            }
            l_string = G.node_types[i].name + " (" + G.node_types[i].short_name + ")";
            r_string = std::to_string(G.node_types[i].number) + " [first = "
                        + std::to_string(G.node_types[i].first);
            r_string += ", last = " + std::to_string(G.node_types[i].last - 1) + ", alg. repr. = ";
            r_string += vector_to_a_string(G.node_types[i].alg_rep_range, 0) + ", length = ";
            r_string += std::to_string(G.node_types[i].alg_rep) + ", pad_abs = "
                         + std::to_string(G.node_types[i].pad_abs);
            r_string += ", pad_rel = " + std::to_string(G.node_types[i].pad_rel) + ", pad_bit = ";
            r_string += std::to_string(G.node_types[i].pad_bit) + "]";
            write_line({l_string, r_string}, 19, output);
        }
    }
}

/**
*****************************************
Functions concerning the output file name
*****************************************
**/

string get_output_file_name(// supplied data:
                            vector<ValueAndTriple>&    values_and_triples)
/*
Make an output filestring of the form:
 - RA.name[param-names-values-tp_name-RA.mode-RA.nondet_repeat]FAU.suffix-year-mo-da-hrmnsc.txt
Note that 'tp_name' is the name of the tp in the first tp (on the assumption that most of the
time there will be just one tp name).

Functions:
 - fatal            from error_writing
*/
{
    string date_time = current_date_time_suffix();              // time-stamp for an output file
    string file_string = RA.name + "[";
    size_t num_params = IN.param_names.size();
    for (size_t i = 0; i < num_params; i++)                     // include each parameter
    {
        file_string += IN.param_names[i] + "=" + std::to_string(IN.param_values[i]) + ",";
    }
    file_string += TP.expanded[0].name + ",";                   // add on first tp name
    auto it = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                           [&](const ValueAndTriple& vt)
    {
        return vt.value == GlobalParamVal{RA.mode};
    });
    if (it != values_and_triples.end())
        file_string += it->triple.value;
    else
        fatal(Errors::IllegalRoutingMode);
    if (RA.mode == RoutingMode::nondet)
        file_string += std::to_string(RA.nondet_repeat);        // add on num reps if "nondet"
    if (FAU.suffix.empty())
        file_string += "]-" + date_time + ".txt";
    else
        file_string += "]-" + FAU.suffix + "-" + date_time + ".txt";
    return file_string;
}

/**
**********************
File writing functions
**********************
**/

void write_the_output_file_header(// supplied data:
                                  const vector<node>&              int_net,
                                  const vector<ValueAndTriple>&    values_and_triples,
                                  const size_t                     label_width,
                                  const string&                    execution_run_time_string,
                                  // other stuff:
                                  std::ostream&                    output)
/*
Write the prelude to the stats in the output file.

Functions:
 - write_line           from here
*/
{
    write_line({"date and time / elapsed time", current_date_time_nice() + " / " +
               execution_run_time_string}, label_width, output);
    output << "\n";
    string a_string;
    a_string += IN.name + " [";
    for (size_t i = 0; i < IN.param_names.size(); ++i)
    {
        a_string += IN.param_names[i] + "=" + std::to_string(IN.param_values[i]);
        if (i < IN.param_names.size() - 1)
            a_string +=",";
    }
    a_string += "]";
    write_line({"interconnection network", a_string}, label_width, output);

    auto it = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                          [&](const ValueAndTriple& vt)
    {
        return vt.value == GlobalParamVal{RA.mode};
    });
    a_string = RA.name + " [mode=" + it->triple.value;
    if (RA.mode == RoutingMode::nondet)
        a_string += std::to_string(RA.nondet_repeat);
    a_string += "]";
    if (RA.valiant)
        a_string += " with Valiant routing";
    write_line({"routing algorithm", a_string}, label_width, output);
    if (NOT.added_note != "")
        write_line({"notes", NOT.added_note}, label_width, output);
    output << "\n";
    write_line({"fault seeds", vector_to_a_string(FAU.seeds, 0)}, label_width, output);
    write_line({"traffic pattern seeds", vector_to_a_string(TP.seeds, 0)}, label_width, output);
    write_line({"routing seeds", vector_to_a_string(RA.seeds, 0)}, label_width, output);
    write_line({"miscellaneous seeds", vector_to_a_string(IN.seeds, 0)}, label_width, output);

    output << "\nfault descriptions:\n";
    for (const auto &fd : FAU.fault_description)
    {
        output << " - <" << fd.name << "," << fd.percentage << "%," << fd.raw_count << ",\""
            << fd.source_node_type << "\",\"" << fd.dest_node_type << "\"> with intended number of faults "
            << fd.number_of_faults << "\n";
    }

    output << "\ninput fault files:\n";
    for (size_t i = 0; i < FAU.input_fault_files.size(); ++i)
    {
        write_line({" - input fault file #" + std::to_string(i + 1), FAU.input_fault_files[i]}, label_width,
                   output);
    }

    size_t c = 0;
    for (size_t i = 0; i < TP.batches.size() - 1; ++i)
    {
        output << "\ntraffic patterns";
        if (TP.batches.size() > 2)
            output << " batch #" + std::to_string(i + 1) + ":\n";
        else
            output << ":\n";
        while (c < TP.batches[i + 1])
        {
            write_line({"- traffic pattern #" + std::to_string(c + 1), TP.expanded[c].name + " ("
                       + std::to_string(TP.expanded[c].num_iterations) + " iterations)"}, label_width,
                       output);
            write_line({"  start-source", partial_node(int_net, TP.expanded[c].start_source, false)},
                       label_width, output);
            write_line({"  end-source", partial_node(int_net, TP.expanded[c].end_source - 1, false)},
                       label_width, output);
            write_line({"  start-destination", partial_node(int_net, TP.expanded[c].start_dest, false)},
                       label_width, output);
            write_line({"  end-destination", partial_node(int_net, TP.expanded[c].end_dest - 1, false)},
                       label_width, output);
            ++c;
        }
    }
    output << "\n----------------------------------------\n";
}

void write_latency_stats(// supplied data:
                         const RoutingStats&    stats,
                         const size_t           label_width,
                         // other stuff:
                         std::ostream&          output)
/*
Writes the latency stats from 'stats'.

Functions:
 - write_line               from here
*/
{
    double route_percentage = 0.0;
    [[maybe_unused]] double times_tried = 0.0;
    if ((stats.failed_routes + stats.successful_routes) > 0)
    {
        route_percentage = double(stats.successful_routes) * 100.0 /
                                        (stats.failed_routes + stats.successful_routes);
        times_tried = double(stats.times_tried) /
                                        (stats.failed_routes + stats.successful_routes);
    }

    vector<LabelValue> labels_and_values =
    {
        {"total number of routes generated", std::to_string(stats.failed_routes + stats.successful_routes)},
        {"successful routes", std::to_string(stats.successful_routes)},
        {"failed routes", std::to_string(stats.failed_routes)},
        {"percentage successful", format_double(route_percentage, System::dp_precision) + "%"},
        //{"average attempts to find route", format_double(times_tried, System::dp_precision)},
        {"minimum hop-length", std::to_string(stats.latency.minimum)},
        {"maximum hop-length", std::to_string(stats.latency.maximum)},
        {"average hop-length", format_double(stats.latency.average, System::dp_precision)},
        {" - standard deviation", format_double(stats.latency.std_dev, System::dp_precision)},
        {" - coefficient of variation", format_double(stats.latency.coeff_var, System::dp_precision)},
        {"median hop-length", format_double(stats.latency.median, System::dp_precision)}
    };

    for (const auto &lv : labels_and_values)
    {
        write_line(lv, label_width, output);
    }
}

/**
inline std::string stat_to_string(//supplied data:
                                  const TypeOfStat&    val)

Format a double, size_t, a vector of doubles or a vector of size_ts as a string using 'dp_precision'.

Functions:
 - format_double                from misc_formatting_functions
 - vector_to_a_string           from misc_formatting_functions
**/

/**
template <typename StatArray>
TypeOfStat assign_stat(// supplied data:
                       const StatArray&     stats_array,
                       const StatType       tag,
                       const std::size_t    index)

'stats_array' consists of an array of StatBlock objects but of varying sizes. This function returns the
appropriate element from the StatBlock 'stats_array[index]' (of type TypeOfStat, i.e, double or size_t,
vector<double>, vector<size_t>).
**/

/**
template <typename StatArray, typename TypeContainer>
void write_load_act_stats(// supplied data:
                          const StatArray&                  stats_array,
                          const TypeContainer&              types,
                          const std::vector<LabelStat>&     labels_to_print,
                          const std::size_t                 label_width,
                          // other stuff:
                          std::ostream&                     output)

Writes a the stats with their values. The functions converts everything to strings.
 - 'stats_array' = array of, e.g., [pr, se, sw, os, *] or [prpr, prse, ..., osos, *], i.e., node-types or
   channel-types
 - 'types' = array of NodeType of ChannelType of size 'System::num_node_types' or 'System::num_channel_types'
             i.e., 'G.node_types' or 'G.channel_types'
 - 'labels_to_print' = vector of pairs of string/{size_t, double}, e.g., minimum/0, average/3.4
 - 'label_width' = the number of spaces to reserve for a label

Functions:
 - assign_stat         from here
 - stat_to_string      from here
 - write_line          from here
**/

void write_node_load_stats(// supplied data:
                           const std::array<StatBlock, System::num_node_types + 1>&    stats,
                           const size_t                                                label_width,
                           // other stuff:
                           std::ostream&                                               output)
/*
Write the node load stats.

Functions:
 - write_load_act_stats         from here
*/
{
    vector<LabelStat> node_labels =
    {
        {"minimum node load", StatType::minimum},
        {"maximum node load", StatType::maximum},
        {"average node load", StatType::average},
        {" - standard deviation", StatType::std_dev},
        {" - coefficient of variation", StatType::coeff_var},
        {"median node load", StatType::median},
        {"sum of node loads", StatType::sum}
    };
    write_load_act_stats(stats, G.node_types, node_labels, label_width, output);
}

void write_channel_load_stats(// supplied data:
                              const std::array<StatBlock, System::num_channel_types + 1>&    stats,
                              const size_t                                                   label_width,
                              // other stuff:
                              std::ostream&                                                  output)
/*
Write the channel load stats.

Functions:
 - write_load_act_stats         from here
*/
{
    vector<LabelStat> channel_labels =
    {
        {"minimum channel load", StatType::minimum},
        {"maximum channel load", StatType::maximum},
        {"average channel load", StatType::average},
        {" - standard deviation", StatType::std_dev},
        {" - coefficient of variation", StatType::coeff_var},
        {"median channel load", StatType::median},
        {"sum of channel loads", StatType::sum}
    };
    write_load_act_stats(stats, G.channel_types, channel_labels, label_width, output);
}

void write_node_activity_stats(// supplied data:
                               const std::array<StatBlock, System::num_node_types + 1>&    stats,
                               const size_t                                                label_width,
                               // other stuff:
                               std::ostream&                                               output)
/*
Write the node activity stats.

Functions:
 - write_load_act_stats         from here
*/
{
    vector<LabelStat> node_labels =
    {
        {"minimum node activity", StatType::minimum},
        {"maximum node activity", StatType::maximum},
        {"average node activity", StatType::average},
        {" - standard deviation", StatType::std_dev},
        {" - coefficient of variation", StatType::coeff_var},
        {"median node activity", StatType::median},
        {"sum of node activities", StatType::sum}
    };
    write_load_act_stats(stats, G.node_types, node_labels, label_width, output);
}

void write_channel_activity_stats(// supplied data:
                            const std::array<StatBlock, System::num_channel_types + 1>&    stats,
                            const size_t                                                   label_width,
                            // other stuff:
                            std::ostream&                                                  output)
/*
Write the channel activity stats.

Functions:
 - write_load_act_stats         from here
*/
{
    vector<LabelStat> channel_labels =
    {
        {"minimum channel activity", StatType::minimum},
        {"maximum channel activity", StatType::maximum},
        {"average channel activity", StatType::average},
        {" - standard deviation", StatType::std_dev},
        {" - coefficient of variation", StatType::coeff_var},
        {"median channel activity", StatType::median},
        {"sum of channel activity", StatType::sum}
    };
    write_load_act_stats(stats, G.channel_types, channel_labels, label_width, output);
}

void write_node_timed_activity_stats(// supplied data:
                    const std::array<TimedActivityStatBlock, System::num_node_types + 1>&    stats,
                    const size_t                                                             label_width,
                    // other stuff:
                    std::ostream&                                                            output)
/*
Write the node activity stats.

Functions:
 - write_load_act_stats         from here
*/
{
    vector<LabelStat> node_labels =
    {
        {"minimum node timed activity", StatType::minimum},
        {"maximum node timed activity", StatType::maximum},
        {"average node timed activity", StatType::average},
        {" - standard deviation", StatType::std_dev},
        {" - coefficient of variation", StatType::coeff_var},
        {"median node timed activity", StatType::median},
        {"sum of node timed activities", StatType::sum}
    };
    write_load_act_stats(stats, G.node_types, node_labels, label_width, output);
}

void write_channel_timed_activity_stats(// supplied data:
                const std::array<TimedActivityStatBlock, System::num_channel_types + 1>&    stats,
                const size_t                                                                label_width,
                // other stuff:
                std::ostream&                                                               output)
/*
Write the channel activity stats.

Functions:
 - write_load_act_stats         from here
*/
{
    vector<LabelStat> channel_labels =
    {
        {"minimum channel timed activity", StatType::minimum},
        {"maximum channel timed activity", StatType::maximum},
        {"average channel timed activity", StatType::average},
        {" - standard deviation", StatType::std_dev},
        {" - coefficient of variation", StatType::coeff_var},
        {"median channel timed activity", StatType::median},
        {"sum of channel timed activity", StatType::sum}
    };
    write_load_act_stats(stats, G.channel_types, channel_labels, label_width, output);
}

/**
*****************
Write output file
*****************
**/

void write_the_final_output_file(//supplied data:
                                 vector<node>&                    int_net,
                                 const vector<ValueAndTriple>&    values_and_triples,
                                 const size_t                     l_string_pad,
                                 const string&                    overall_time_string,
                                 vector<RoutingStats>&            batch_stats,
                                 vector<RoutingStats>&            tp_stats,
                                 std::ostringstream&              buffer,
                                 // other stuff:
                                 std::ostream&                    output_file)
/*
Write all the output to the output file.

Functions:
 - write_the_output_file_header                 from here
 - write_latency_stats                          from here
 - write_node_load_stats                        from here
 - write_channel_load_stats                     from here
 - write_node_activity_stats                    from here
 - write_channel_activity_stats                 from here
 - write_node_timed_activity_stats              from here
 - write_channel_timed_activity_stats           from here
*/
{
    write_the_output_file_header(int_net, values_and_triples, l_string_pad, overall_time_string, output_file);

    for (size_t b = 0; b < TP.batches.size() - 1; ++b)
    {
        if (TP.batches.size() > 2)
            output_file << "\nBATCH #" << b + 1 << " OVERALL RESULTS\n\n";
        else
            output_file << "\nOVERALL RESULTS\n\n";
        output_file << "LATENCY:\n";
        write_latency_stats(batch_stats[b], l_string_pad, output_file);
        output_file << "\nNODE LOADS:\n";
        write_node_load_stats(batch_stats[b].node_load, l_string_pad, output_file);
        output_file << "\nCHANNEL LOADS:\n";
        write_channel_load_stats(batch_stats[b].channel_load, l_string_pad, output_file);
        if (ENV.sync)
        {
            output_file << "\nNODE ACTIVITIES:\n";
            write_node_activity_stats(batch_stats[b].node_activity, l_string_pad, output_file);
            output_file << "\nCHANNEL ACTIVITIES:\n";
            write_channel_activity_stats(batch_stats[b].channel_activity, l_string_pad, output_file);
            output_file << "\nNODE TIMED ACTIVITIES:\n";
            write_node_timed_activity_stats(batch_stats[b].node_timed_activity, l_string_pad, output_file);
            output_file << "\nCHANNEL TIMED ACTIVITIES:\n";
            write_channel_timed_activity_stats(batch_stats[b].channel_timed_activity, l_string_pad, output_file);
        }

        if (!ENV.suppress_tps)
        {
            if ((TP.batches[b + 1] - TP.batches[b]) > 1)
            {
                for (size_t tp = TP.batches[b]; tp < TP.batches[b + 1]; ++tp)
                {
                    output_file << "\nINDIVIDUAL TRAFFIC PATTERN #" << tp + 1;
                    if (TP.batches.size() > 2)
                        output_file << " FROM BATCH #" << b + 1;
                    output_file << "\n\n";
                    output_file << "LATENCY:\n";
                    write_latency_stats(tp_stats[tp], l_string_pad, output_file);
                    output_file << "\nNODE LOADS:\n";
                    write_node_load_stats(tp_stats[tp].node_load, l_string_pad, output_file);
                    output_file << "\nCHANNEL LOADS:\n";
                    write_channel_load_stats(tp_stats[tp].channel_load, l_string_pad, output_file);
                    if (ENV.sync)
                    {
                        output_file << "\nNODE ACTIVITIES:\n";
                        write_node_activity_stats(tp_stats[tp].node_activity, l_string_pad, output_file);
                        output_file << "\nCHANNEL ACTIVITIES:\n";
                        write_channel_activity_stats(tp_stats[tp].channel_activity, l_string_pad, output_file);
                        output_file << "\nNODE TIMED ACTIVITIES:\n";
                        write_node_timed_activity_stats(tp_stats[tp].node_timed_activity, l_string_pad, output_file);
                        output_file << "\nCHANNEL TIMED ACTIVITIES:\n";
                        write_channel_timed_activity_stats(tp_stats[tp].channel_timed_activity, l_string_pad, output_file);
                    }
                }
            }
        }
    }
    if (!buffer.str().empty())
    {
        output_file << "\n----------------------------------------\n";
        output_file << buffer.str();
    }
}











