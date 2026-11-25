
#include <filesystem>
#include <algorithm>
#include <fstream>

#include "write_faults_to_file.h"
#include "../globals.h"
#include "../misc/misc_formatting_functions.h"

using std::vector;
using std::string;
using std::size_t;
using std::array;
using std::pair;

namespace fs = std::filesystem;


/**
This cpp contains functions concerning writing faults to a file. Before a set of faults is written to a
fault file, the faults are partitioned according to type and there is an option to sort the faults.

Functions prior to writing the faults:
 - partition_faults()
 - sort_faults_and_tokenize()
 - compute_numbers_of_faults()
Writing the faults:
 - write_faults_to_file()
**/

/**
*************************************
Functions prior to writing the faults
*************************************
**/

void partition_faults(// supplied data:
                      const vector<node>&                                  int_net,
                      FaultBatch&                                          batch_of_faults,
                      // computed data:
                      array<vector<size_t>, System::num_node_types>&       array_of_faults_nodes,
                      array<vector<size_t>, System::num_channel_types>&    array_of_faults_channels,
                      array<vector<size_t>, System::num_channel_types>&    array_of_faults_ports,
                      // other stuff:
                      std::ostream&                                        output)
/*
Given lists of faults in 'batch_of_faults' (in absolute id form), partition faults according to type. The
partitioned faults will be further processed, and may be sorted, as tokens before writing to file in
write_faults_to_file() . Note that the sizes of 'array_of_faults_channels' and 'array_of_faults_ports'
are identical. It is assumed that all supplied faults are valid and that there are no repetitions.
*/
{
    size_t node_type;
    size_t channel_type;

    output << "\npartitioning faults ...\n";

    size_t num_node_faults = batch_of_faults.nodes.size();
    size_t num_channel_faults = batch_of_faults.channels.size();

    for (size_t i = 0; i < System::num_node_types; ++i)                 // reserve space for node lists
    {
        array_of_faults_nodes[i].clear();
        array_of_faults_nodes[i].reserve(num_node_faults);              // over-estimate space needed
    }
    for (size_t i = 0; i < num_node_faults; ++i)                        // insert nodes appropriately
    {
        node_type = int_net[batch_of_faults.nodes[i]].node_get_type();  // get node type
        array_of_faults_nodes[node_type].push_back(batch_of_faults.nodes[i]);
    }
    for (size_t i = 0; i < System::num_channel_types; ++i)              // reserve space for channel lists
    {
        array_of_faults_channels[i].clear();
        array_of_faults_channels[i].reserve(num_channel_faults);        // over-estimate space needed
        array_of_faults_ports[i].clear();
        array_of_faults_ports[i].reserve(num_channel_faults);           // over-estimate space needed
    }
    for (size_t i = 0; i < num_channel_faults; ++i)                     // insert nodes appropriately
    {
        node_type = int_net[batch_of_faults.channels[i]].node_get_type();// get source node type
        channel_type = int_net[batch_of_faults.channels[i]].node_get_port_node(System::out,
                                                        batch_of_faults.ports[i]);// get target node
        channel_type = int_net[channel_type].node_get_type();           // get target type
        channel_type = nn_to_c(node_type, channel_type);                // get channel type
        array_of_faults_channels[channel_type].push_back(batch_of_faults.channels[i]);
        array_of_faults_ports[channel_type].push_back(batch_of_faults.ports[i]);
    }

    output << "... faults partitioned\n";
}

void update_fault_numbers(const size_t                                         tp_number,
                          // computed data:
                          array<vector<size_t>, System::num_node_types>&       array_of_faults_nodes,
                          array<vector<size_t>, System::num_channel_types>&    array_of_faults_channels)
/*
Update 'TP.expanded[tp_number].num_faulty_nodes[*]' and 'TP.expanded[tp_number].num_faulty_channels[*]'.
This registers the true numbers of faults per tp.
*/
{
    size_t total_faulty_nodes = 0;
    for (size_t i = 0; i < System::num_node_types; ++i)
    {
        TP.expanded[tp_number].num_faulty_nodes[i] = array_of_faults_nodes[i].size();
        total_faulty_nodes += array_of_faults_nodes[i].size();
    }
    TP.expanded[tp_number].num_faulty_nodes[System::num_node_types] = total_faulty_nodes;
    size_t total_faulty_channels = 0;
    for (size_t i = 0; i < System::num_channel_types; ++i)
    {
        TP.expanded[tp_number].num_faulty_channels[i] = array_of_faults_channels[i].size();
        total_faulty_channels += array_of_faults_channels[i].size();
    }
    TP.expanded[tp_number].num_faulty_channels[System::num_channel_types] = total_faulty_channels;
}

void sort_faults_and_tokenize(// supplied data:
                              const vector<node>&                                  int_net,
                              array<vector<size_t>, System::num_node_types>&       array_of_faults_nodes,
                              array<vector<size_t>, System::num_channel_types>&    array_of_faults_channels,
                              array<vector<size_t>, System::num_channel_types>&    array_of_faults_ports,
                              // computed data:
                              array<vector<string>, System::num_node_types>&       array_of_tokens_nodes,
                              array<vector<string>, System::num_channel_types>&    array_of_tokens_channels,
                              // other stuff:
                              std::ostream&                                        output)
/*
Given partitioned lists of faults (in absolute id form), sort these lists (if requested) and convert the
faults to tokens. Note that tokens are strings, e.g., pr45 where 45 is a relative id. These lists are used
to store faults in fault files. It is assumed that all supplied faults are valid and that there are no
repetitions.
*/
{
    // fault sorting

    if (FAU.sort_faults)
    {
        output << "\nsorting faults ...\n";

        for (size_t i = 0; i < System::num_node_types; ++i)         // over all node types
        {
            std::sort(array_of_faults_nodes[i].begin(), array_of_faults_nodes[i].end());
        }

        for (size_t i = 0; i < System::num_channel_types; ++i)      // over all channel types
        {
            size_t num_channel_faults = array_of_faults_channels[i].size();
            vector<pair<std::size_t, size_t>> xy;                   // list used to sort absid
            xy.reserve(num_channel_faults);                         //  channels-ports in-place
            for (size_t j = 0; j < num_channel_faults; ++j)
            {
                xy.emplace_back(array_of_faults_channels[i][j], array_of_faults_ports[i][j]);
            }
            std::sort(xy.begin(), xy.end());                        // sort lexicographically
            for (size_t j = 0; j < num_channel_faults; ++j)         // split the absid chan-port pairs
            {
                array_of_faults_channels[i][j] = xy[j].first;
                array_of_faults_ports[i][j] = xy[j].second;
            }
        }

        output << "... faults sorted\n";
    }

    // fault tokenizing

    output << "\ntokenizing faults ...\n";

    for (size_t nt = 0; nt < System::num_node_types; ++nt)          // over all node types
    {
        size_t num_nodes = array_of_faults_nodes[nt].size();
        array_of_tokens_nodes[nt].clear();
        array_of_tokens_nodes[nt].reserve(num_nodes);
        for (size_t j = 0; j < num_nodes; ++j)
        {
            size_t relative_id = array_of_faults_nodes[nt][j] - G.node_types[nt].first;
            array_of_tokens_nodes[nt].push_back(G.node_types[nt].short_name + std::to_string(relative_id));
        }
    }

    for (size_t ct = 0; ct < System::num_channel_types; ++ct)       // over all channel types
    {
        size_t num_channels = array_of_faults_channels[ct].size();
        array_of_tokens_channels[ct].clear();
        array_of_tokens_channels[ct].reserve(num_channels);
        for (size_t j = 0; j < num_channels; ++j)
        {
            size_t node_type = int_net[array_of_faults_channels[ct][j]].node_get_type();
            size_t relative_id = array_of_faults_channels[ct][j] - G.node_types[node_type].first;
            array_of_tokens_channels[ct].push_back(G.node_types[node_type].short_name
                                                   + std::to_string(relative_id) + "-"
                                                   + std::to_string(array_of_faults_ports[ct][j]));
        }
    }

    output << "... faults tokenized\n";
}

/**
template <typename T>
void compute_numbers_of_faults(// supplied data:
                    std::array<std::vector<T>, System::num_node_types>&           array_of_node_faults,
                    std::array<std::vector<T>, System::num_channel_types>&        array_of_channel_faults,
                    // computed data:
                    std::array<std::vector<T>, System::num_node_types + 1>&       numbers_of_node_faults,
                    std::array<std::vector<T>, System::num_channel_types + 1>&    numbers_of_channel_faults)

Compute the numbers of faults of the different types, including overall, given the lists of the current
faults. Works for fault lists of various types.
**/

/**
******************
Writing the faults
******************
**/

void write_faults_to_file(// supplied data:
                          const vector<node>&                                  int_net,
                          const size_t                                         modified_tp_number,
                          const string&                                        input_fault_file,
                          const string&                                        fault_imposition_time_string,
                          array<vector<size_t>, System::num_node_types>&       array_of_faults_nodes,
                          array<vector<size_t>, System::num_channel_types>&    array_of_faults_channels,
                          array<vector<size_t>, System::num_channel_types>&    array_of_faults_ports,
                          // other stuff:
                          std::ostream &output)
/*
Writes fault data for a traffic pattern or batch to the display or a file. The actual tp-number needs to
be supplied so as to build an output file name. Note that 'modified_tp_number' should run 1, 2, ... with a
value of 0 meaning that the tp-number should not feature in the fault file name. Note that this function
does nothing if 'FAU.output_fault_file' = "false". It is assumed that all faults supplied are valid and
have been partitioned and that there are no repetitions.

Functions:
 - sort_faults_and_tokenize         from here
 - current_date_time_suffix         from misc_formatting_functions
*/
{
    if (FAU.output_fault_file.empty())               // do nothing if told not to
        return;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    string file_string;
    size_t total_node_faults;
    size_t total_channel_faults;
    string token;

    // build output fault file name

    if (FAU.output_fault_file == "true")                // default name
    {
        if (modified_tp_number > 0)
            file_string = "fault_file_" + current_date_time_suffix() + "-"
                            + std::to_string(modified_tp_number);
        else
            file_string = "fault_file_" + current_date_time_suffix();
        if (FAU.suffix.empty())
            file_string = file_string + ".txt";
        else
            file_string = file_string + "-" + FAU.suffix + ".txt";
    }
    else                                                // given name (which is not "false" or empty)
    {
        if (modified_tp_number > 0)
            file_string = FAU.output_fault_file + "-" + std::to_string(modified_tp_number);
        else
            file_string = FAU.output_fault_file;
        if (FAU.suffix.empty())
            file_string = file_string + ".txt";
        else
            file_string = file_string + "-" + FAU.suffix + ".txt";
    }

    // sort the faults and tokenize prior to writing to file

    array<vector<string>, System::num_node_types>    array_of_tokens_nodes;
    array<vector<string>, System::num_channel_types> array_of_tokens_channels;

    sort_faults_and_tokenize(int_net, array_of_faults_nodes, array_of_faults_channels,
                             array_of_faults_ports, array_of_tokens_nodes, array_of_tokens_channels,
                             output);

    // write faults to file

    std::ofstream of_stream;            // output file stream
    fs::path path_to_file = fs::current_path() / fs::path("ACTUAL_NETWORKS") / fs::path(IN.name)
                                / fs::path("fault_files") / fs::path(file_string);// build path to file
    if (!fs::exists(path_to_file.parent_path())) // if folder "fault_files" does not exist in int net folder
        fs::create_directory(path_to_file.parent_path());//  create it
    of_stream.open(path_to_file, std::ios::out);        // try and open file
    if (of_stream)
    {
        of_stream << "Fault file created at " << current_date_time_nice() << " for " << IN.name << " [";
        for (size_t i = 0; i < IN.param_names.size(); ++i)
        {
            of_stream << IN.param_names[i] << "=" << IN.param_values[i];
            if (i < IN.param_names.size() - 1)
                of_stream << ",";
            else
                of_stream << "]\n\n";
        }
        of_stream << "These faults were made by ";
        if (!FAU.fault_description.empty())
        {
            of_stream << "imposing the faults in the following descriptions:\n";
            for (const auto &fd : FAU.fault_description)
            {
                of_stream << " - <" << fd.name << "," << fd.percentage << "%," << fd.raw_count << ",\""
                    << fd.source_node_type << "\",\"" << fd.dest_node_type
                    << "\"> with intended number of faults " << fd.number_of_faults << "\n";
            }
        }
        if (!FAU.input_fault_files.empty())
        {
            if (!FAU.fault_description.empty())
                of_stream << " and ";
            of_stream << "imposing the faults in the following fault file:\n";
            of_stream << " - " << input_fault_file << "\n";
        }

        total_node_faults = 0;
        for (size_t i = 0; i < G.num_node_types; ++i)
            total_node_faults = total_node_faults + array_of_tokens_nodes[i].size();
        total_channel_faults = 0;
        for (size_t i = 0; i < G.num_channel_types; ++i)
            total_channel_faults = total_channel_faults + array_of_tokens_channels[i].size();

        of_stream << "The total numbers of faults set are:\n";
        of_stream << " faulty nodes: " << total_node_faults << "\n";
        for (size_t i = 0; i < G.num_node_types; ++i)
        {
            if (array_of_tokens_nodes[i].size() > 0)
            {
                of_stream << "  - " << array_of_tokens_nodes[i].size() << " "
                    << G.node_types[i].name << " faults\n";
            }
        }
        of_stream << " faulty channels: " << total_channel_faults << "\n";
        for (size_t i = 0; i < G.num_channel_types; ++i)
        {
            if (array_of_tokens_channels[i].size() > 0)
            {
                of_stream << "  - " << array_of_tokens_channels[i].size() << " "
                    << G.channel_types[i].name << " faults\n";
            }
        }
        of_stream << "Fault seeds: " << vector_to_a_string(FAU.seeds, 0) << "\n";
        of_stream << "Time to impose faults: " << fault_imposition_time_string << "\n\n";

        of_stream << "<NODE>\n";
        for (size_t k = 0; k < System::num_node_types; ++k)
        {
            for(size_t i = 0; i < array_of_tokens_nodes[k].size(); ++i)
            {
                of_stream << array_of_tokens_nodes[k][i];
                if (i < array_of_tokens_nodes[k].size() - 1)
                    of_stream << ",";
                else
                    of_stream << "\n";
            }
        }

        of_stream << "<CHANNEL>\n";
        for (size_t k = 0; k < System::num_channel_types; ++k)
        {
            for (size_t i =0; i < array_of_tokens_channels[k].size(); ++i)
            {
            of_stream << array_of_tokens_channels[k][i];
            if (i < array_of_tokens_channels[k].size() - 1)
                of_stream << ",";
            else
                of_stream << "\n";
            }
        }

        std::chrono::high_resolution_clock::time_point stop = std::chrono::high_resolution_clock::now();
        string fault_processing_writing_time_string = get_elapsed_time(start, stop);

        of_stream << "\nTime to process and write faults: "
            << fault_processing_writing_time_string << "\n";

        of_stream << "<END OF FILE>\n";
        of_stream.close();                        // close the output file
    }
    else
        warning(Errors::CantOpenFile, file_string);
}
