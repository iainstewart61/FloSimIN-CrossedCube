
#include "crossedcube.h"
#include "../../globals.h"
#include "../../construction/int_net_construction.h"

using std::vector;
using std::size_t;

/**
In order to introduce a new interconnection network, the following functions need to be supplied:
 - <int_net>_set_globals()
 - <int_net>_alg_convert_to_id()
 - <int_net>_alg_convert_from_id()
 - <int_net>_build().
Other functions can be supplied too but they will be local to this cpp. If there is some data that
is best computed once only and made available for use thereafter then this should be defined in
the anonymous namespace.

Note that we include error_writing.h. A user can cover themselves for coding errors and should use
the general errors Errors::BuildingError and Errors::ExecutionError.
**/

namespace
/**
This anonymous namespace enables ready access to the dimension n > 0 which is always defined via the
instruction:
 - size_t n = IN.param_values[0];
in <int_net>_set_globals(). Other values can be included too, e.g.,
 - for k-ary n-cubes, n and k might appear in the anonymous namespace
with the additional instructions
 - k = IN.param_values[1];
in <int_net>_set_globals().
**/
{
    // LEAVE INSTRUCTIONS AS ARE

    size_t n;

    // USER-DEFINED

    // discretionary aliases can be added below to make life easier for the user, e.g.,
    //     const auto& pr = G.pr;
    //     const auto& proc = G.node_types[pr];

}

size_t swap_0_and_1(// supplied data:
                    const size_t    a_size_t)
/*
Swap a 1 and a 0.
*/
{
    if (a_size_t == 0)
        return 1;
    else
        return 0;
}

void twiddles(// supplied data:
              size_t             j,
              // supplied/computed data:
              vector<size_t>&    the_node_alg)
/*
Given the tuple 'the_node_alg', the two bits at locations j and j + 1 are 'twiddled' as per the definition
of the crossed cube. So, the call only makes sense for an even j (as bits are named 0, 1, 2, ...).
*/
{
    if (the_node_alg[j] == 1)
        the_node_alg[j + 1] = swap_0_and_1(the_node_alg[j + 1]);// (j + 1)th bit flipped between 0 and 1
}                                                               //  if jth bit = 1

void crossedcube_set_globals()
/**
This function necessarily sets the following globals as follows (example shown is hypercubes):
 - G.node_types.number
   - only need to set for those node types with > 0 nodes
     e.g., G.node_types[G.pr].number = size_t{1} << n;
 - G.node_types.alg_rep
   - only need to set for those node types with > 0 nodes
     e.g., G.node_types[G.pr].alg_rep = n;
 - G.node_types.alg_rep_range
   - only need to set for those node types with > 0 nodes
     e.g., G.node_types[G.pr].alg_rep_range.resize(n, 2);
 - G.channel_types.number
   - only need to set for those channel types with > 0 channels
     e.g., G.channel_types[nn_to_c(G.pr,G.pr)].number = n * G.node_types[G.pr].number;
   - the helper function
      - nn_to_c: (node type, node type) -> channel type
     from structs_system.h should be used for cartesian-defined INs

Functions:
 - fatal            from error_writing
**/
{
    // LEAVE INSTRUCTIONS AS ARE

    n = IN.param_values[0];                     // the dimension n is always 'IN.param_values[0]'

    // USER-DEFINED
    // for each node/channel type where there are nodes/channels

    if (n >= sizeof(size_t) * 8)
        fatal(Errors::DimensionTooLarge);
    G.node_types[G.pr].number                  = size_t{1} << n; // set # of processors = 2^n
    G.node_types[G.pr].alg_rep                 = n;             // processors are n-bit tuples
    G.node_types[G.pr].alg_rep_range.resize(n, 2);// set every dimensional range to 2 (note that
                                                  //  'G.node_types[G.pr].alg_rep_range' starts as empty)
    G.channel_types[nn_to_c(G.pr,G.pr)].number = n * G.node_types[G.pr].number;// set # of pr-pr channels
}

Errors crossedcube_alg_convert_to_id(// supplied data:
                                     const size_t             node_type,
                                     const vector<size_t>&    the_alg_id,
                                     // computed data:
                                     size_t&                  rel_id)
/**
Converts a given algebraic id 'the_alg_id' of a given node type 'node_type' into a *relative* numeric id
'rel_id'. The following should hold:
 - if 'the_alg_id.size() != G.node_types[node_type].alg_rep' then
   - an error BadLengthTuple should be returned
 - if some element 'the_alg_id[i]' is >= 'G.node_types[node_type].alg_rep_range[i]' then
   - an error OutOfRangeTuple should be returned
 - otherwise the corresponding relative id of the node in question should be returned as 'rel_id'.
We need this function for each interconnection network as not all INs are cartesian-defined, though if an
IN is cartesian-defined then the function cart_alg_convert_to_id() from int_net_construction should be
used. If an error is returned by this function then 'rel_id' is garbage.

Errors:
 - Good, BadLengthTuple, OutOfRangeTuple

Functions:
 - cart_alg_convert_to_id           from int_net_construction
**/
{
    // LEAVE INSTRUCTIONS AS ARE

    if (the_alg_id.size() != G.node_types[node_type].alg_rep)// look for BadLengthTuple error
        return Errors::BadLengthTuple;
    for (size_t i = 0; i < n; ++i)                  // look for OutOfRangeTuple error
    {
        if (the_alg_id[i] >= G.node_types[node_type].alg_rep_range[i])
            return Errors::OutOfRangeTuple;
    }

    // USER-DEFINED

    rel_id = cart_alg_convert_to_id(the_alg_id, G.node_types[node_type].alg_rep_range);
    return Errors::Good;
}

Errors crossedcube_alg_convert_from_id(// supplied data:
                                       const size_t       node_type,
                                       const size_t       the_rel_id,
                                       // computed data:
                                       vector<size_t>&    the_alg_id)
/**
Converts a *relative* id 'the_rel_id' into an algebraic id 'the_alg_id' of node type 'node_type'. The
following should hold:
 - if 'the_rel_id >= G.num_nodes[node_type].number' then
   - an error OutOfRangeRelId should be returned
 - otherwise the corresponding algebraic id is returned as 'the_alg_id'.
We need this function for each interconnection network as not all INs are cartesian-defined, though if
an IN is cartesian-defined then the function 'cart_alg_convert_from_id()' from int_net_construction
should be used. If an error is returned by this function then 'the_alg_id' is garbage. We assume that
'the_alg_id' has had space reserved.

Errors:
 - Good, OutOfRangeRelId

Functions:
 - cart_alg_convert_from_id         from int_net_construction
**/
{
    // LEAVE INSTRUCTIONS AS ARE

    if (the_rel_id >= G.node_types[node_type].number)
        return Errors::OutOfRangeRelId;

    // USER-DEFINED

    the_alg_id = cart_alg_convert_from_id(the_rel_id, G.node_types[node_type].alg_rep_range);
    return Errors::Good;
}

void crossedcube_build(// computed data:
                       vector<node>&    int_net)
/**
Build a vector of processors 'vector<node> int_net'. Building is in two phases:
 - first, the number of ports must be set and the two vectors of input and output ports for each node
   must be built using the function
    * 'int_net[*].node_set_ports(node-type, num-ports]'
 - second, the chnanels need to be defined using the function
    * 'int_net[*].node_set_node_and_port(o-port, target-node, i-port).
As regards the second phase, its easiest to cycle through the nodes setting all the output ports at
the node and the input ports at the target node. Note that an 'int_net' consists of a vector of nodes
of size 'G.total_nodes'. If an 'int_net' has, e.g., servers and switches then care must be taken to
respect the node ordering in 'int_net' when defining channels.

Functions:
 - crossedcube_alg_convert_to_id            from here
**/
{
    // LEAVE INSTRUCTIONS AS ARE

    [[maybe_unused]] Errors error;

	// USER-DEFINED PHASE 1 - set 'node_num_ports' and build 'node_in_port' and 'node_out_port'

	for (size_t from_node = G.node_types[G.pr].first; from_node < G.node_types[G.pr].last; from_node++)
    {
        int_net[from_node].node_set_ports(G.pr, n);         // set the number of ports and build the ports
    }

    // USER-DEFINED PHASE 2 - build channels

    for (size_t from_node = G.node_types[G.pr].first; from_node < G.node_types[G.pr].last; from_node++)
    {
        size_t to_node;
        vector<size_t> the_node_alg;
        the_node_alg = int_net[from_node].node_get_alg();   // algebraic identifier of 'from_node'
        for (size_t the_port = 0; the_port < n; the_port++) // do each port in turn
        {
            vector<size_t> original = the_node_alg;
            the_node_alg[the_port] = (original[the_port] == 1 ? 0 : 1);
            for (size_t j = 0; j + 1 < the_port; j += 2)    // twiddle to get dim-'the_port' neighbour
                twiddles(j, the_node_alg);
            error = crossedcube_alg_convert_to_id(G.pr, the_node_alg, to_node);
            the_node_alg = original;
            int_net[from_node].node_set_node_and_port(the_port, to_node, the_port);// build channel
        }
    }
}
