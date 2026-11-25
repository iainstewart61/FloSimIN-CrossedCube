
#ifndef VALIDATION_H
#define VALIDATION_H

#include <vector>
#include <string>
#include <fstream>

#include "data_structures_for_config.h"
#include "read_registry_file.h"

void validate_core_int_net_data(// supplied data:
                                TheRegistry&     the_registry,
                                // other stuff:
                                std::ostream&    output);

void validate_traffic_patterns(// supplied/computed data:
                               std::vector<Tag>&    config_tags);

void validate_fault_descriptions();

void rewrite_the_registry(// supplied data:
                          const std::string&              input_file_name,
                          std::vector<ValueAndTriple>&    values_and_triples,
                          std::string&                    pre_registry,
                          std::string&                    post_registry,
                          TheRegistry&                    the_registry);

#endif
