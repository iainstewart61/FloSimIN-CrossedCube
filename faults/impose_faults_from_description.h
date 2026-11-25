
#ifndef IMPOSE_FAULTS_FROM_DESCRIPTION_H
#define IMPOSE_FAULTS_FROM_DESCRIPTION_H

#include <iostream>
#include <vector>
#include <string>
#include <cstddef>

#include "../node.h"
#include "../structs/structs_fault.h"

/**
*******************************
Functions involving node faults
*******************************
**/

std::size_t guess_a_possible_source(// supplied data:
                                    const std::string&    node_type);

/**
*************************************
Impose faults from fault descriptions
*************************************
**/

bool test_node(// supplied data:
               std::vector<node>&    int_net,
               const std::size_t     source);

bool test_channel(// supplied data:
                  const std::vector<node>&    int_net,
                  const std::size_t           source,
                  const std::size_t           source_port);

void impose_faults_description(// supplied data:
                               std::vector<node>&    int_net,
                               // computed data:
                               FaultBatch&           batch_of_faults,
                               // other stuff:
                               std::ostream&         output);

#endif
