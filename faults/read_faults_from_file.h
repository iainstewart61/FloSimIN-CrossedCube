
#ifndef READ_FAULTS_FROM_FILE_H
#define READ_FAULTS_FROM_FILE_H

#include <iostream>
#include <string>
#include <vector>

#include "../node.h"
#include "../structs/structs_fault.h"

/**
********************
Reading a fault file
********************
**/

void read_faults_from_file(// supplied data:
                           const std::vector<node>&    int_net,
                           const std::string&          the_input_fault_file,
                           // computed data:
                           FaultBatch&                 the_faults_read,
                           // other stuff:
                           std::ostream&               output);

/**
****************
Fault management
****************
**/

void impose_list_of_faults(// supplied data:
                           std::vector<node>&        int_net,
                           const FaultBatch&         the_faults_read,
                           // supplied/computed data:
                           FaultBatch&               batch_of_faults);

void remove_all_faults(// supplied data:
                       std::vector<node>&    int_net);

#endif
