
#ifndef EXECUTION_H
#define EXECUTION_H

#include <vector>
#include <sstream>
#include <ostream>

#include "node.h"
#include "input/data_structures_for_config.h"

void execution(// supplied data:
               std::vector<node>&              int_net,
               const int                       l_string_pad,
               std::vector<ValueAndTriple>&    values_and_triples,
               std::ostringstream&             buffer,
               // other stuff:
               std::ostream&                   output);

#endif
