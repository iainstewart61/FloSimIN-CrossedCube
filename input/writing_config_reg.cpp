
#include "writing_config_reg.h"
#include "read_registry_file.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp file contains functions to write the data read and validated from config and registry.
The main function is:
 - write_globals().

Writing a single line:
 - write_line()
 - is_default()
 - write_line_wrt_default()
Writing traffic patterns:
 - write_int_tp()
 - write_expanded_tp()
Writing fault descriptions:
 - write_fd()
Writing the contents of the registry:
 - write_contents_registry()
Writing globals:
 - write_system_globals()
 - write_globals()
**/

/**
*********************
Writing a single line
*********************
**/

/**
template <typename T>
void write_line(//supplied data:
                const std::string&                    left_string,
                int                                   left_pad,
                const T&                              right_structure,
                const std::vector<ValueAndTriple>&    values_and_triples,
                // other stuff:
                std::ostream&                         output)

Writes:
 - 'truncate_or_pad(left_string, left_pad) + ': ' + 'right_string' + '\n'
where
 - 'right_string' = 'right_structure' if the type is not user-defined
 - 'right_string' = appropriate string-value if the type is user-defined.

Functions:
 - truncate_or_pad              from misc_formatting_functions
 - coding                       from error_writing
 - vector_to_a_string           from misc_formatting_functions
*/

bool is_default(// supplied data:
                const vector<Tag>&    config_tags,
                const string&         main_tag,
                const string&         var)
/*
Return whether a given variable 'var' from the main tag 'main_tag' has been given a value (false) or whether
it retains its default (true). Also, we assume that 'main_tag' and 'var' form a legitimate pair. If they
don't then we terminate with a coding error.
*/
{
    bool found_pair = false;
    bool was_value_read;
    for (const auto& tag : config_tags)
    {
        if (tag.name == main_tag)
        {
            for (const auto& subtag : tag.subtags)
            {
                if (subtag.name == var)
                {
                    found_pair = true;
                    was_value_read = subtag.value_read;
                    break;
                }
            }
            break;
        }
    }
    if (found_pair == false)
        coding(Errors::IllegalTagSubtagPair);
    return was_value_read;
}

/**
template <typename T>
void write_line_wrt_default(//supplied data:
                            const std::string&                    main_tag,
                            const std::string&                    var,
                            std::string                           left_string,
                            int                                   left_pad,
                            const T&                              right_structure,
                            const std::vector<ValueAndTriple>&    values_and_triples,
                            // other stuff:
                            std::ostream&                         output)

If ENV.view_globals == ViewGlobals::all and the value is non-default then write:
 - 'truncate_or_pad(left_string + '*', left_pad) + ': ' + 'right_string' + '\n'
if ENV.view_globals == ViewGlobals::all and the value is default then writes:
 - 'truncate_or_pad(left_string, left_pad) + ': ' + 'right_string' + '\n'
if ENV.view_globals == ViewGlobals::nondef and the value is non-default then writes:
 - 'truncate_or_pad(left_string, left_pad) + ': ' + 'right_string' + '\n'
if ENV.view_globals == ViewGlobals::nondef and the value is default then writes nothing
where:
 - 'right_string' = 'right_structure' if the type is not user-defined
 - 'right_string' = appropriate string-value if the type is user-defined.

Functions:
 - is_default               from here
 - write_line               from here
**/

/**
************************
Writing traffic patterns
************************
**/

void write_int_tp(// supplied data:
                  const TrafficPatternInterim&    interim_tp,
                  // other stuff:
                  std::ostream&                   output)
/*
Write an interim traffic pattern.
*/
{
    output << "  [" << interim_tp.name << ", " << interim_tp.num_iterations
    << ", " << interim_tp.num_repeats << "]<" << interim_tp.percentage << "%>{";
    for (size_t i = 0; i < 4; ++i)
    {
        if (interim_tp.sources_dests[i].prefix == "default")
            output << "default";
        else
        {
            if (interim_tp.sources_dests[i].prefix != "*")
                output << interim_tp.sources_dests[i].prefix;
            if (std::holds_alternative<std::size_t>(interim_tp.sources_dests[i].id_or_tuple))
                output << std::get<std::size_t>(interim_tp.sources_dests[i].id_or_tuple);
            else
            {
                vector<size_t> value = std::get<vector<size_t>>(interim_tp.sources_dests[i].id_or_tuple);
                output << "[";
                for (size_t j = 0; j < value.size(); ++j)
                {
                    output << value[j];
                    if (j != value.size() - 1)
                        output << ",";
                }
                output << "]";
            }
        }
        if (i != 3)
            output << ";";
    }
    output << "}\n";
}

void write_expanded_tp(// supplied data:
                    const TrafficPatternExpanded&    expanded_tp,
                    // other stuff:
                    std::ostream&                    output)
/*
Writes an expanded traffic pattern.
*/
{
    output << "   [" << expanded_tp.name << ", " << expanded_tp.num_iterations << ", "
        << expanded_tp.start_source << ", " << expanded_tp.end_source - 1 << ", "
        << expanded_tp.start_dest << ", " << expanded_tp.end_dest - 1 << "]\n";
}

/**
**************************
Writing fault descriptions
**************************
**/

void write_fd(// supplied data:
              const FaultDescription&    fd,
              // other stuff:
              std::ostream&              output)
/*
Write a fault description.
*/
{
    output << "  [" << fd.name;
    output << ", " << fd.source_node_type;
    output << ", " << fd.dest_node_type;
    output << "]";
    if (fd.raw_count == 0)
        output << "<" << fd.percentage << "%>\n";
    else
        output << "<" << fd.raw_count << ">\n";
}

/**
************************************
Writing the contents of the registry
************************************
**/

void write_contents_registry(// supplied stuff:
                             const int                        left_pad,
                             const TheRegistry&               the_registry,
                             const vector<ValueAndTriple>&    values_and_triples,
                             // other stuff:
                             std::ostream&                    output)
/*
Write contents of registry.

Functions:
 - write_line               from here
*/
{
    output << "\n*** CONTENTS OF REGISTRY ***\n";
    output << "\nINTERCONNECTION NETWORKS\n";
    for (const auto& reg_int_net : the_registry.int_nets_registered)
    {
        output << "\n " << reg_int_net.name << ":\n";
        write_line("  dimension_names", left_pad, reg_int_net.parameters, values_and_triples, output);
        write_line("  net_machine", left_pad, reg_int_net.net_machine, values_and_triples, output);
        write_line("  net_type", left_pad, reg_int_net.net_type, values_and_triples, output);
        write_line("  net_build", left_pad, reg_int_net.net_build, values_and_triples, output);
        auto it = std::find_if(the_registry.notes_registered.begin(), the_registry.notes_registered.end(),
                            [&](const RegNote& item)
                            {
                                return item.name == reg_int_net.name;
                            });
        write_line("  registry_note", left_pad, it->note, values_and_triples, output);
    }

    output << "\nTRAFFIC PATTERNS\n\n";
    for (const auto &tp : the_registry.traffic_patterns_registered)
    {
        output << " " << tp << "\n";
    }
}

/**
***************
Writing globals
***************
**/

void write_system_globals(// supplied data:
                         const int                        left_pad,
                         const vector<ValueAndTriple>&    values_and_triples,
                         // other stuff:
                         std::ostream&                    output)
/*
View all the system global variables.

Functions:
 - write_line               from here
*/
{
    string final_string;
    output << "\n*** SYSTEM GLOBALS ***\n\n";
    write_line("num_term_types", left_pad, G.num_term_types, values_and_triples, output);
    write_line("pr", left_pad, G.pr, values_and_triples, output);
    write_line("se", left_pad, G.se, values_and_triples, output);
    write_line("num_switch_types", left_pad, G.num_switch_types, values_and_triples, output);
    write_line("sw", left_pad, G.sw, values_and_triples, output);
    write_line("os", left_pad, G.os, values_and_triples, output);
    write_line("num_node_types", left_pad, G.num_node_types, values_and_triples, output);
    write_line("num_channel_types", left_pad, G.num_channel_types, values_and_triples, output);
    output << "node types:\n";
    for (const auto& nt : G.node_types)
    {
        final_string = nt.name + "/" + nt.short_name + "/";
        if (nt.terminal == true)
            final_string += "terminal";
        else
            final_string += "non-terminal";
        write_line(" name/short_name/terminal", left_pad, final_string, values_and_triples, output);
    }
    output << "channel types:\n";
    for (const auto& ct : G.channel_types)
    {
        write_line(" name/short_name", left_pad, ct.name + "/" + ct.short_name,
                   values_and_triples, output);
    }
    write_line("in", left_pad, System::in, values_and_triples, output);
    write_line("out", left_pad, System::out, values_and_triples, output);
    write_line("used", left_pad, G.used, values_and_triples, output);
    write_line("unused", left_pad, G.unused, values_and_triples, output);
    write_line("healthy", left_pad, G.healthy, values_and_triples, output);
    write_line("faulty", left_pad, G.faulty, values_and_triples, output);
    write_line("dp_precision", left_pad, G.dp_precision, values_and_triples, output);
    write_line("num_seeds", left_pad, G.num_seeds, values_and_triples, output);
    write_line("view_system", left_pad, G.view_system, values_and_triples, output);
    write_line("view_registry", left_pad, G.view_registry, values_and_triples, output);
}

void write_globals(// supplied stuff:
                   const vector<Tag>&               config_tags,
                   const int                        left_pad,
                   const TheRegistry&               the_registry,
                   const vector<ValueAndTriple>&    values_and_triples,
                   // other stuff:
                   std::ostream&                    output)
/*
View all or non-default global variables or don't view any depending upon 'ENV.view_globals'.

Functions:
 - write_line                   from here
 - write_line_wrt_default       from here
 - write_int_tp                 from here
 - write_expanded_tp            from here
 - write_fd                     from here
 - write_system_globals         from here
 - write_contents_registry      from here
*/
{
    if (ENV.view_globals == ViewGlobals::none)
        return;
    if (ENV.view_globals == ViewGlobals::nondef)
        output << "\n*** GLOBAL VARIABLES SET IN CONFIG ***\n";
    else
        output << "\n*** ALL GLOBAL VARIABLES ***\n";

    output << "\n<INTERCONNECTION NETWORK>\n";
    write_line(" name", left_pad, IN.name, values_and_triples, output);
    write_line(" param names", left_pad, IN.param_names, values_and_triples, output);
    write_line(" param values", left_pad, IN.param_values, values_and_triples, output);
    write_line_wrt_default(config_tags, "<INTERCONNECTIONNETWORK>", "seeds", " seeds", left_pad, IN.seeds,
                           values_and_triples, output);

    output  << "\n<ROUTING ALGORITHM>\n";
    write_line(" name", left_pad, RA.name, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "dor", " dor", left_pad, RA.dor,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "tiebreak", " tiebreak", left_pad, RA.tiebreak,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "mode", " mode", left_pad, RA.mode,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "nondet_repeat", " nondet_repeat", left_pad,
                           RA.nondet_repeat, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "increm_bound", " increm_bound", left_pad,
                           RA.increm_bound, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "valiant", " valiant", left_pad,
                           RA.valiant, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ROUTINGALGORITHM>", "seeds", " seeds", left_pad, RA.seeds,
                           values_and_triples, output);

    output  << "\n<TRAFFIC PATTERN>\n";
    write_line_wrt_default(config_tags, "<TRAFFICPATTERN>", "batches", " batches", left_pad, TP.batches,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<TRAFFICPATTERN>", "seeds", " seeds", left_pad, TP.seeds,
                           values_and_triples, output);
    output << " interim traffic patterns:\n";
    for (const auto &interim_tp : TP.interim)
    {
        write_int_tp(interim_tp, output);
    }
    output << " expanded traffic patterns:\n";
    size_t j = 0;
    for (size_t i = 0; i < TP.expanded.size(); ++i)
    {
        if ((i == TP.batches[j]) && (j < TP.batches.size()))
        {
            output << "  batch " << j << ":\n";
            ++j;
        }
        write_expanded_tp(TP.expanded[i], output);
    }

    output  << "\n<ENVIRONMENT>\n";
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "sync", " sync", left_pad, ENV.sync,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "sync_worm", " sync_worm", left_pad, ENV.sync_worm,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "suppress_tps", " suppress_tps", left_pad,
                           ENV.suppress_tps, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "use_used_mode", " use_used_mode", left_pad,
                           ENV.use_used_mode, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "view_routes", " view_routes", left_pad,
                           ENV.view_routes, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "view_int_net", " view_int_net", left_pad,
                           ENV.view_int_net, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "view_int_net_with_faults",
                           " view_int_net_with_faults", left_pad, ENV.view_int_net_with_faults,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "view_loads", " view_loads", left_pad, ENV.view_loads,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "view_globals", " view_globals", left_pad,
                           ENV.view_globals, values_and_triples, output);
    write_line_wrt_default(config_tags, "<ENVIRONMENT>", "rewrite_registry", " rewrite_registry", left_pad,
                           ENV.rewrite_registry, values_and_triples, output);

    output  << "\n<FAULT>\n";
    write_line_wrt_default(config_tags, "<FAULT>", "fault_check", " fault_check", left_pad, FAU.fault_check,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "input_fault_files", " input_fault_files", left_pad,
                           FAU.input_fault_files, values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "output_fault_file", " output_fault_file", left_pad,
                           FAU.output_fault_file, values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "seeds", " seeds", left_pad, FAU.seeds, values_and_triples,
                           output);
    write_line_wrt_default(config_tags, "<FAULT>", "limit_guesses", " limit_guesses", left_pad,
                           FAU.limit_guesses, values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "show_generation", " show_generation", left_pad,
                           FAU.show_generation, values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "sort_faults", " sort_faults", left_pad, FAU.sort_faults,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "regenerate", " regenerate", left_pad, FAU.regenerate,
                           values_and_triples, output);
    write_line_wrt_default(config_tags, "<FAULT>", "suffix", " suffix", left_pad, FAU.suffix,
                           values_and_triples, output);
    if (!FAU.fault_description.empty())
    {
        output << " fault descriptions:\n";
        for (const auto &fd : FAU.fault_description)
        {
            write_fd(fd, output);
        }
    }

    output  << "\n<LATENCY>\n";
    write_line_wrt_default(config_tags, "<LATENCY>", "refined_latency", " refined_latency", left_pad,
                           LAT.refined_latency, values_and_triples, output);
    write_line_wrt_default(config_tags, "<LATENCY>", "stack_latency", " stack_latency", left_pad,
                           LAT.stack_latency, values_and_triples, output);
    write_line_wrt_default(config_tags, "<LATENCY>", "propagation_latency", " propagation_latency",
                           left_pad, LAT.propagation_latency, values_and_triples, output);
    write_line_wrt_default(config_tags, "<LATENCY>", "data_transfer_latency", " data_transfer_latency",
                           left_pad, LAT.data_transfer_latency, values_and_triples, output);
    write_line_wrt_default(config_tags, "<LATENCY>", "average_routing_latency", " average_routing_latency",
                           left_pad, LAT.average_routing_latency, values_and_triples, output);
    write_line_wrt_default(config_tags, "<LATENCY>", "per_hop_latency", " per_hop_latency", left_pad,
                           LAT.per_hop_latency, values_and_triples, output);
    write_line_wrt_default(config_tags, "<LATENCY>", "os_recon_latency", " os_recon_latency", left_pad,
                           LAT.os_recon_latency, values_and_triples, output);

    output  << "\n<BANDWIDTH>\n";
    write_line_wrt_default(config_tags, "<BANDWIDTH>", "port_bandwidth", " port_bandwidth", left_pad,
                           BAN.port_bandwidth, values_and_triples, output);

    output  << "\n<PACKET>\n";
    write_line_wrt_default(config_tags, "<PACKET>", "packet_size", " packet_size", left_pad,
                           PKT.packet_size, values_and_triples, output);
    write_line_wrt_default(config_tags, "<PACKET>", "header_size", " header_size", left_pad,
                           PKT.header_size, values_and_triples, output);

    output  << "\n<NOTES>\n";
    write_line_wrt_default(config_tags, "<NOTES>", "added_note", " added_note", left_pad,
                           NOT.added_note, values_and_triples, output);

    output << "\nREGISTRY\n";
    write_line(" registry_note", left_pad, RG.registry_note, values_and_triples, output);
    write_line(" net_machine", left_pad, RG.net_machine, values_and_triples, output);
    write_line(" net_type", left_pad, RG.net_type, values_and_triples, output);
    write_line(" net_build", left_pad, RG.net_build, values_and_triples, output);

    // perhaps view systems globals

    if (G.view_system)
        write_system_globals(left_pad, values_and_triples, output);

    // perhaps view complete registry

    if (G.view_registry)
        write_contents_registry(left_pad, the_registry, values_and_triples, output);
}
