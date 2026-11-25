
#ifndef WRITE_FAULTS_TO_FILE_H
#define WRITE_FAULTS_TO_FILE_H

#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <cstddef>

#include "../node.h"
#include "../structs/structs_fault.h"
#include "../structs/structs_system.h"

/**
*************************************
Functions prior to writing the faults
*************************************
**/

void partition_faults(// supplied data:
                const std::vector<node>&                                       int_net,
                FaultBatch&                                                    batch_of_faults,
                // computed data:
                std::array<std::vector<std::size_t>, System::num_node_types>&       array_of_faults_nodes,
                std::array<std::vector<std::size_t>, System::num_channel_types>&    array_of_faults_channels,
                std::array<std::vector<std::size_t>, System::num_channel_types>&    array_of_faults_ports,
                // other stuff:
                std::ostream&                                                  output);

void update_fault_numbers(// supplied data:
            const std::size_t                                                   tp_number,
            // computed data:
            std::array<std::vector<std::size_t>, System::num_node_types>&       array_of_faults_nodes,
            std::array<std::vector<std::size_t>, System::num_channel_types>&    array_of_faults_channels);

template <typename T>
void compute_numbers_of_faults(// supplied data:
                    std::array<std::vector<T>, System::num_node_types>&           array_of_node_faults,
                    std::array<std::vector<T>, System::num_channel_types>&        array_of_channel_faults,
                    // computed data:
                    std::array<std::vector<T>, System::num_node_types + 1>&       numbers_of_node_faults,
                    std::array<std::vector<T>, System::num_channel_types + 1>&    numbers_of_channel_faults)
/*
Compute the numbers of faults of the different types, including overall, given the lists of the current
faults. Works for fault lists of various types.
*/
{
    numbers_of_node_faults.fill(0);
    for (std::size_t i = 0; i < System::num_node_types; ++i)
    {
        numbers_of_node_faults[i] = array_of_node_faults[i].size();
        numbers_of_node_faults[System::num_node_types] = numbers_of_node_faults[System::num_node_types]
                                                            + numbers_of_node_faults[i];
    }
    numbers_of_channel_faults.fill(0);
    for (std::size_t i = 0; i < System::num_channel_types; ++i)
    {
        numbers_of_channel_faults[i] = array_of_channel_faults[i].size();
        numbers_of_channel_faults[System::num_channel_types] =
                    numbers_of_channel_faults[System::num_channel_types] + numbers_of_channel_faults[i];
    }
}

/**
******************
Writing the faults
******************
**/

void write_faults_to_file(// supplied data:
            const std::vector<node>&                                            int_net,
            const std::size_t                                                   modified_tp_number,
            const std::string&                                                  input_fault_file,
            const std::string&                                                  fault_imposition_time_string,
            std::array<std::vector<std::size_t>, System::num_node_types>&       array_of_faults_nodes,
            std::array<std::vector<std::size_t>, System::num_channel_types>&    array_of_faults_channels,
            std::array<std::vector<std::size_t>, System::num_channel_types>&    array_of_faults_ports,
            // other stuff:
            std::ostream &output);

#endif
