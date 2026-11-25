
#ifndef WRITING_OUTPUT_H
#define WRITING_OUTPUT_H

#include <string>
#include <vector>
#include <cstddef>

#include "../misc/misc_formatting_functions.h"
#include "../node.h"
#include "../structs/structs_routing.h"
#include "../structs/structs_routing_stats.h"
#include "../input/data_structures_for_config.h"

/**
*******
Structs
*******
**/

struct LabelValue
{
    std::string    label;
    std::string    value;
};

enum class StatType
{
    minimum, maximum, average, std_dev, coeff_var, median, sum
};

struct LabelStat
{
    std::string label;
    StatType stat;
};

using TypeOfStat = std::variant<double, std::size_t, std::vector<double>, std::vector<std::size_t>>;

/**
**********************************
Functions for initializing spacing
**********************************
**/

void compute_pad_values_for_writing();

/**
******************************************************************
Functions for writing individual nodes, channels, routes and lines
******************************************************************
**/

std::string partial_node(// supplied data:
                         const std::vector<node>&    int_net,
                         const std::size_t           abs_id,
                         const bool                  reverse_it);

std::string write_a_node_entry(// supplied data:
                               const std::vector<node>&    int_net,
                               const std::size_t           abs_id);

std::string write_a_channel_entry(// supplied data:
                                  const std::vector<node>&    int_net,
                                  const std::size_t           abs_id_src,
                                  const std::size_t           port_src);

void view_a_route(// supplied data:
                  const std::vector<node>&    int_net,
                  const std::size_t           source,
                  const std::size_t           dest,
                  const RouteStruct&          route,
                  // other stuff:
                  std::ostream&               output);

void write_line(// supplied data:
                const LabelValue&         lv,
                const std::size_t         label_width,
                // other stuff:
                std::ostream&             output);

/**
**********************
Overall view functions
**********************
**/

void view_nodes_and_channels(// supplied data:
                             const std::vector<node>&    int_net,
                             // other stuff:
                             std::ostream&               output);

void view_int_net_topology_numerics(// supplied data:
                                    const int l_string_pad,
                                    // other stuff:
                                    std::ostream &output);

/**
*****************************************
Functions concerning the output file name
*****************************************
**/

std::string get_output_file_name(// supplied data:
                                 std::vector<ValueAndTriple>&    values_and_triples);

/**
**********************
File writing functions
**********************
**/

void write_the_output_file_header(// supplied data:
                                  const std::vector<node>&              int_net,
                                  const std::vector<ValueAndTriple>&    values_and_triples,
                                  const std::size_t                     label_width,
                                  const std::string&                    execution_run_time_string,
                                  // other stuff:
                                  std::ostream&                         output);

void write_latency_stats(// supplied data:
                         const RoutingStats&    stats,
                         const std::size_t      label_width,
                         // other stuff:
                         std::ostream&          output);

inline std::string stat_to_string(//supplied data:
                                  const TypeOfStat&    val)
/*
Format a double, size_t, a vector of doubles or a vector of size_ts as a string using 'dp_precision'.

Functions:
 - format_double                from misc_formatting_functions
 - vector_to_a_string           from misc_formatting_functions
*/
{
    return std::visit([](const auto &v) -> std::string
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, double>)
            return format_double(v, System::dp_precision);

        else if constexpr (std::is_same_v<T, std::size_t>)
            return std::to_string(v);

        else if constexpr (std::is_same_v<T, std::vector<double>>)
            return vector_to_a_string(v, 0);

        else if constexpr (std::is_same_v<T, std::vector<std::size_t>>)
            return vector_to_a_string(v, 0);

        else
            throw std::logic_error("Unknown stat type");
    }, val);
}

template <typename StatArray>
TypeOfStat assign_stat(// supplied data:
                       const StatArray&     stats_array,
                       const StatType       tag,
                       const std::size_t    index)
/*
'stats_array' consists of an array of StatBlock objects but of varying sizes. This function returns the
appropriate element from the StatBlock 'stats_array[index]' (of type TypeOfStat, i.e, double, size_t,
vector<double>, vector<size_t>).
*/
{
    switch (tag)                            // decide which element to point at using 'tag'
    {
        case StatType::minimum:   return stats_array[index].minimum;
        case StatType::maximum:   return stats_array[index].maximum;
        case StatType::average:   return stats_array[index].average;
        case StatType::std_dev:   return stats_array[index].std_dev;
        case StatType::coeff_var: return stats_array[index].coeff_var;
        case StatType::median:    return stats_array[index].median;
        case StatType::sum:       return stats_array[index].sum;
    }
    throw std::logic_error("Invalid StatType");
}

template <typename StatArray, typename TypeContainer>
void write_load_act_stats(// supplied data:
                          const StatArray&                  stats_array,
                          const TypeContainer&              types,
                          const std::vector<LabelStat>&     labels_to_print,
                          const std::size_t                 label_width,
                          // other stuff:
                          std::ostream&                     output)
/*
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
*/
{
    // collect indices of types that exist (number > 0)
    std::vector<std::size_t> indices_to_print;
    for (std::size_t i = 0; i < types.size(); ++i)
    {
        if (types[i].number > 0)
            indices_to_print.push_back(i);
    }

    LabelValue lv;          // the label/value as a pair of strings

    // loop through labels and types
    for (const auto &label : labels_to_print)
    {
        for (std::size_t idx : indices_to_print)
        {
            lv.label = label.label + " - " + types[idx].short_name;
            TypeOfStat val = assign_stat(stats_array, label.stat, idx);
            lv.value = stat_to_string(val);
            write_line(lv, label_width, output);
        }

        // overall across types if more than one
        if (indices_to_print.size() > 1)
        {
            lv.label = label.label + " - overall";
            TypeOfStat val = assign_stat(stats_array, label.stat, types.size());
            lv.value = stat_to_string(val);
            write_line(lv, label_width, output);
        }
    }
}

void write_node_load_stats(// supplied data:
                           const std::array<StatBlock, System::num_node_types + 1>&    stats,
                           const std::size_t                                           label_width,
                           // other stuff:
                           std::ostream&                                               output);

void write_channel_load_stats(// supplied data:
                              const std::array<StatBlock, System::num_channel_types + 1>&    stats,
                              const std::size_t                                              label_width,
                              // other stuff:
                              std::ostream&                                                  output);

void write_node_activity_stats(// supplied data:
                               const std::array<StatBlock, System::num_node_types + 1>&    stats,
                               const std::size_t                                           label_width,
                               // other stuff:
                               std::ostream&                                               output);

void write_channel_activity_stats(// supplied data:
                                  const std::array<StatBlock, System::num_channel_types + 1>&    stats,
                                  const std::size_t                                              label_width,
                                  // other stuff:
                                  std::ostream&                                                  output);

void write_node_timed_activity_stats(// supplied data:
                    const std::array<TimedActivityStatBlock, System::num_node_types + 1>&    stats,
                    const std::size_t                                                        label_width,
                    // other stuff:
                    std::ostream&                                                            output);

void write_channel_timed_activity_stats(// supplied data:
                    const std::array<TimedActivityStatBlock, System::num_channel_types + 1>&    stats,
                    const std::size_t                                                           label_width,
                    // other stuff:
                    std::ostream&                                                               output);

/**
*****************
Write output file
*****************
**/

void write_the_final_output_file(//supplied data:
                                 std::vector<node>&                    int_net,
                                 const std::vector<ValueAndTriple>&    values_and_triples,
                                 const std::size_t                     l_string_pad,
                                 const std::string&                    overall_time_string,
                                 std::vector<RoutingStats>&            batch_stats,
                                 std::vector<RoutingStats>&            tp_stats,
                                 std::ostringstream&                   buffer,
                                 // other stuff:
                                 std::ostream&                         output_file);
#endif
