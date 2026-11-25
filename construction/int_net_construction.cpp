
#include <algorithm>

#include "int_net_construction.h"
#include "../globals.h"
#include "../errors/error_writing.h"

using std::vector;
using std::string;
using std::size_t;
using std::pair;
using std::make_pair;

/**
This ccp file contains functions related to interconnection network construction and
node and channel usage.

Functions relating to cartesian <-> algebraic conversion
 - cart_alg_get_next()
 - cart_alg_convert_to_id()
 - cart_alg_convert_from_id()
Functions relating to channel and route manipulations
 - get_channel_type_and_target_id()
 - ports_between_nodes()
 - fault_check_route()
Functions relating to interconnection network construction
 - validate_int_net_globals()
 - topology_numerics()
 - prelude_to_int_net_build()
 - postlude_to_int_net_build()
**/

/**
********************************************************
Functions relating to cartesian <-> algebraic conversion
********************************************************
**/

void cart_alg_get_next(// supplied data:
                       const vector<size_t>&    dim_bounds,
                       // supplied/computed data:
                       vector<size_t>&          the_alg_id)
/*
Returns the cartesian algebraic identifier after the supplied one with the return in the same variable
'the_alg_id'. The ordering is the usual lexicographic ordering. The ordering is from the right, i.e.,
bit num-dims from bits 1, 2, ..., num-dims is the least significant. If 'the_alg_id' = [max, max, ...,
max] then the next algebraic identifier is wrapped around and set to [0, 0, ..., 0].
*/
{
    size_t num_dims = the_alg_id.size();
	size_t bit_to_increment = num_dims;     // look for bit that needs incrementing: default 'num_dims'
	for (size_t i = num_dims; i-- > 0; )    // cycle through components from the right
    {
        if (the_alg_id[i] != dim_bounds[i] - 1)// register if current component is the bit-to-increment
		{
			bit_to_increment = i;           // least significant bit on the right
			break;
		}
    }
	if (bit_to_increment != num_dims)		// if we are not at the last algebraic identifier ...
    {
        the_alg_id[bit_to_increment]++;		// increment the bit-to-increment
		for (size_t i = bit_to_increment + 1; i < num_dims; ++i)// cycle through the remaining
        {                                                       //  components and reset each at 0
            the_alg_id[i] = 0;
        }
    }
	else                                    // if we are at the last algebraic identifier ...
	{
		for (size_t i = 0; i < num_dims; ++i)// reset the algebraic identifier to zero
        {
            the_alg_id[i] = 0;
        }
	}
}

size_t cart_alg_convert_to_id(// supplied data:
                              const vector<size_t>&    the_alg_id,
                              const vector<size_t>&    dim_bounds)
/*
Converts an algebraic identifier into a numeric identifier. The least significant bit is on the right,
i.e., bit num-dims from bits 1, 2, ..., num-dims. The ordering is the usual lexicographic ordering. If
we have 'the_alg_id' = [x_0, x_1, ..., x_{n-1}] from [0..k_0-1, 0..k_1-1, ..., 0..k_{n-1}-1], given by
'dim_bounds', then the formula for 'the_numeric_id' is:
 - x_{n-1} + x_{n-2}k_{n-1} + x_{n-3}k_{n-2}k_{n-1} + x_{n-4}k_{n-3}k_{n-2}k_{n-1} + ...
    + x_0k_{1}k_{2}...k_{n-1}.
Note that the highest value is:
 - (k_0-1)k_1...k_{n-1} + (k_1-1)k_2...k_{n-1} + (k_2-1)k_3...k_{n-1} + .. + (k_{n-2}-1)k_{n-1}
     + (k_{n-1}-1) = k_0k_1...k_{n-1} - 1 as required.
We always assume that 'the_alg_id' is legitimate given 'dim_bounds'.
*/
{
    size_t num_dims = the_alg_id.size();
    size_t the_rel_id = the_alg_id[num_dims - 1];  // set as x_{n-1}
    size_t prefix = 1;
    for (size_t i = num_dims - 1; i-- > 0; )            // note: condition checked before decrementing i
    {                                                   //  for loop (so it starts with i = 'num_dims' - 2)
        prefix = prefix * dim_bounds[i + 1];            // in general, 'prefix' = k_{i+1}k_{i+2}...k_{n-1}
        the_rel_id = the_rel_id + prefix * the_alg_id[i];// in general, add on x_ik_{i+1}k_{i+2}
    }                                                    //  ...k_{n-1}
    return the_rel_id;
}

vector<size_t> cart_alg_convert_from_id(// supplied data:
                                        const size_t             the_relative_id,
                                        const vector<size_t>&    dim_bounds)
/*
Converts a relative numeric identifier into an algebraic identifier. The least significant bit is on the
right, i.e., bit num-dims from bits 1, 2, ..., num-dims. The ordering is the usual lexicographic ordering.
If we have 'the_rel_id' =
 - x_{n-1} + x_{n-2}k_{n-1} + x_{n-3}k_{n-2}k_{n-1} + x_{n-4}k_{n-3}k_{n-2}k_{n-1} + ...
    + x_0k_{1}k_{2}...k_{n-1}
where [0..k_0-1, 0..k_1-1, ..., 0..k_{n-1}-1] is given by 'dim_bounds', then
 - 'the_alg_id' = [x_0, x_1, ..., x_{n-1}] from [0..k_0-1, 0..k_1-1, ..., 0..k_{n-1}-1].
So: divide by
 - k_{n-1} to get rem. x_{n-1} and quot. x_{n-2} + x_{n-3}k_{n-2} + x_{n-4}k_{n-3}k_{n-2} ...
then divide by
 - k_{n-2} to get rem. x_{n-2} and quot. x_{n-3} + x_{n-4}k_{n-3} ... etc.
*/
{
    size_t num_dims = dim_bounds.size();
    vector<size_t> alg_id;
    alg_id.reserve(num_dims);
    size_t remainder;
    size_t quotient = the_relative_id;
    for (size_t i = num_dims; i-- > 0; )            // compute the reverse algebraic id
    {
        remainder = quotient % dim_bounds[i];
        quotient = (quotient - remainder) / dim_bounds[i];
        alg_id.push_back(remainder);
    }
    std::reverse(alg_id.begin(), alg_id.end());     // reverse 'alg_id' (needs <algorithm>)
    return alg_id;
}

/**
*****************************************************
Functions relating to channel and route manipulations
*****************************************************
**/

void get_channel_type_and_target_id(// supplied data:
                                    const vector<node>&    int_net,
                                    const size_t           source_abs_id,
                                    const size_t           source_port,
                                    // computed data:
                                    size_t&                the_channel_type,
                                    size_t&                target_abs_id)
/*
Given an absolute numeric id 'source_abs_id' for a node and an output port 'source_port', return the type
of the channel along with the absolute id of the target node.

Functions called:
 - nn_to_c          from structs_system
*/
{
    size_t source_type;                                              // the type of 'source_abs_id'
    size_t target_type;                                              // the type of a neighbouring node
    source_type = int_net[source_abs_id].node_get_type();
    target_abs_id = int_net[source_abs_id].node_get_port_node(System::out, source_port);
    target_type = int_net[target_abs_id].node_get_type();
    the_channel_type = nn_to_c(source_type, target_type);
}

pair<pair<std::optional<size_t>, std::optional<size_t>>, pair<std::optional<size_t>, std::optional<size_t>>>
ports_between_nodes(//supplied data:
                    const vector<node>&    int_net,
                    const size_t           source_node,
                    const size_t           target_node)
/*
If there is a channel from 'source_node' to 'target_node' then
 - '<o_source, i_target>' = <channel's o-port at 'source_node', channel's i-port at 'target_node'>
and if there is a channel from 'target_node' to 'source_node' then
 - '<o_target, i_source>' = <channel's o-port at 'target_node', channel's i-port at 'source_node'>
where all values are absolute numeric ids. In both cases, otherwise pair is undefined. Note that if 'int_net'
is a multi-network then this function returns only the ports found of lowest index (although I don't really
deal with multi-networks).

The function is used when we need to find the actual channel from one given node to another given node. E.g.,
when setting faults, if we set a link fault and guess a channel to make faulty then we need also to make the
'reverse' channel faulty too. If the 'int_net' has been built symmetrically or part-symmetrically then there
is no need to call this function as <o_source, i_target> = <i_source, o_target>.
*/
{
    std::optional<size_t> o_source;
    std::optional<size_t> i_target;
    std::optional<size_t> o_target;
    std::optional<size_t> i_source;

    size_t num_ports = int_net[source_node].node_get_num_ports();
    for (size_t i = 0; i < num_ports; ++i)// for each
    {                                                                   //  output port at 'source_node' ...
        if (int_net[source_node].node_get_port_node(System::out, i) == target_node)// if port leads to
        {                                                                          //  'target_node' ...
            o_source = i;                                              // set o-port at 'source_node'
            i_target = int_net[source_node].node_get_port_port(System::out, i);// set i-port at 'target_node'
            break;
        }
    }

    num_ports = int_net[target_node].node_get_num_ports();
    for (size_t i = 0; i < num_ports; ++i)// for each
    {                                                                   //  output port at 'target_node' ...
        if (int_net[target_node].node_get_port_node(System::out, i) == source_node)// if port leads to
        {                                                                          //  'source_node' ...
            o_target = i;                                              // set o-port at 'target_node'
            i_source = int_net[target_node].node_get_port_port(System::out, i);// set i-port at 'source_node'
            break;
        }
    }

    return {{o_source, i_target}, {o_target, i_source}};
}

RouteError fault_check_route(//supplied data:
                             const vector<node>&           int_net,
                             const size_t                  source,
                             const size_t                  destination,
                             const struct RouteStruct&     route)
/*
Check whether a given route, in the form {source, <vector of ports>, error-flag>, is a valid route. The
function checks that no node or channel is faulty and that the source and destination are as intended. It
does not matter whether the route has come from a fault tolerant algorithm or not. Note that there is a
comparison between the health of the route and what its error flag says. We might use this function if we
run a non-fault-respecting routing algorithm where there are faults in the interconnection network, and
so ignore them, but take them into account when we have actually computed the route. The code for the
'the_error.code' is as follows:
 - Good = healthy route and all is well
 - FaultySource
 - FaultyNode
 - FaultyChannel
 - BadDestination = the destination in the route is not 'dest' or true destination cannot be reached
 - BadError = the route is fine but the error flag says otherwise
If the error flags a faulty node or channel then 'the_error.location' = i denotes that it is the ith node
or channel where i runs from 0, ..., len if we are talking about nodes and 0 ... len-1 channels, where
len = length of the route = route.port_route.size(). Otherwise the value is 0.
*/
{
    RouteError the_error{Errors::Good, 0};
    size_t length = route.port_route.size();// note: length of a route is the size of the vector of ports
    if (int_net[source].node_get_health() == System::faulty)
    {
        the_error.code = Errors::FaultySource;
        return the_error;
    }
    size_t current_id = source;
    for (size_t i = 0; i < length; i++)
    {
        if (int_net[current_id].node_get_port_health(System::out, route.port_route[i]) == System::faulty)
        {
            the_error.code = Errors::FaultyChannel;
            the_error.location = i;
            return the_error;
        }
        current_id = int_net[current_id].node_get_port_node(System::out, route.port_route[i]);
        if (int_net[current_id].node_get_health() == System::faulty)
        {
            the_error.code = Errors::FaultyNode;
            the_error.location = i + 1;
            return the_error;
        }
    }
    if (current_id != destination)
    {
        the_error.code = Errors::BadDestination;
        return the_error;
    }
    if (route.error_flag.code != Errors::Good)
    {
        the_error.code = Errors::BadError;
        return the_error;
    }
    return the_error;
}

/**
**********************************************************
Functions relating to interconnection network construction
**********************************************************
**/

void validate_int_net_globals()
/*
Check that the globals entered in <int_net>_set_globals() are valid and terminate if not. It is used
only to spot user-errors. It should be called immediately after <int_net>_set_globals().

Functions:
 - fatal            from error_writing
*/
{
    size_t num_nodes = 0;
    for (const auto& nt : G.node_types)
    {
        num_nodes += nt.number;
        if (nt.number > 0)
        {
            if (nt.alg_rep == 0)
                fatal(Errors::IllDefinedAlgRep, nt.name);
            for (size_t i = 0; i < nt.alg_rep; ++i)
            {
                if (nt.alg_rep_range[i] == 0)
                    fatal(Errors::IllDefinedAlgRep, nt.name);
            }
        }
    }
    if (num_nodes == 0)
        fatal(Errors::NoComponents, "no nodes");
}

void topology_numerics(// other stuff:
                       std::ostream &output)
/*
Computes numeric topology values that are dependent upon other values (this function must appear in main
immediately after <int_net>_set_globals() has been called and validated by validate_int_net_globals()). The
global variables computed here need to be:
 - 'G.num_terms'
 - 'G.num_switches
 - 'G.total_nodes'
 - 'G.total_channels'
 - 'G.node_types[*].first'
 - 'G.node_types[*].last'
This function kills execution if there are no terminals or no channels.

Functions:
 - fatal            from error_writing
*/
{
    output << "\nsetting topology numerics ...\n";
    G.num_terms = 0;
    for (size_t ntype = 0; ntype < G.num_term_types; ++ntype)
        G.num_terms += G.node_types[ntype].number;
    G.num_switches = 0;
    for (size_t ntype = G.num_term_types; ntype < G.num_node_types; ++ntype)
        G.num_switches += G.node_types[ntype].number;
    if (G.num_terms == 0)                                       // sanity check
        fatal(Errors::NoComponents, "no nodes");
    G.total_nodes = G.num_terms + G.num_switches;
    for (size_t ntype = 0; ntype < G.num_node_types; ++ntype)
    {
        if (ntype == 0)
        {
            G.node_types[0].first = 0;
            G.node_types[0].last = G.node_types[0].number;
        }
        else
        {
            G.node_types[ntype].first = G.node_types[ntype - 1].last;
            G.node_types[ntype].last = G.node_types[ntype].first + G.node_types[ntype].number;
        }
    }
    G.total_channels = 0;
    for (size_t chtype = 0; chtype < G.num_channel_types; chtype++)
    {
        G.total_channels += G.channel_types[chtype].number;
    }
    if (G.total_channels == 0)                                  // sanity check!
        fatal(Errors::NoComponents, "no channels");
    output << " ... topology numerics set\n";
}

void prelude_to_int_net_build(// supplied/computed data:
                              vector<node>&    int_net)
/*
This function needs to be called immediately after 'int_net' is declared and before 'int_net' is built via
<int_net>_build(). It populates 'int_net' with the correct number of dummy nodes. The reason is that when
'int_net' is built, it needs reference to absolute ids of other nodes. Actually, not all components of the
dummy nodes are dummy:
 - the absolute id
 - the node type
 - the algebraic representation
are all properly set.

Functions:
 - <int_net>_alg_convert_from_id            from <int_net>
*/
{
    Maps::AlgFromNumericFn convert_from_id_ptr;
    convert_from_id_ptr = M.alg_from_numeric[IN.name];

    // set up a vector of the correct number of partially-initialized nodes

    [[maybe_unused]] Errors error;
    int_net.reserve(G.total_nodes);                         // reserve space
    size_t abs_id = 0;                                      // counter works through all nodes
    vector<size_t> the_node_alg;                            // initialize vector for alg rep
    for (size_t ntype = 0; ntype < System::num_node_types; ++ntype)// for each node type ...
    {
        the_node_alg.resize(G.node_types[ntype].alg_rep, 0);// dummy alg. rep. of correct size for 'ntype'
        for (size_t j = G.node_types[ntype].first; j < G.node_types[ntype].last; ++j)// note: never use 'j'
        {
            error = convert_from_id_ptr(ntype, abs_id, the_node_alg);// no need to check 'error'
            node a_new_processor(abs_id, ntype, the_node_alg);// these 3 variables are now correct
            int_net.push_back(a_new_processor);
            int_net[abs_id].node_set_parent_net(&int_net);
            abs_id++;
        }
    }
}

void postlude_to_int_net_build()
/*
This function must be called immediately after the interconnection network has been built via
<int_net>_build(). It checks to see that there is at least 1 node and at least 1 channel. It is purely a
safety-checking function.

Functions:
 - fatal            from error_writing
*/
{
    bool found = false;
    for (const auto &nt : G.node_types)
    {
        if (nt.number > 0)
        {
            found = true;
            break;
        }
    }
    if (found == false)
        fatal(Errors::NoComponents, "no nodes");
    if (G.total_nodes == 0)
        fatal(Errors::NoComponents, "no nodes");

   // check the channel type data structure and 'G.total_channels' to see that there is at least one channel

    found = false;
    for (const auto &ct : G.channel_types)
    {
        if (ct.number > 0)
        {
            found = true;
            break;
        }
    }
    if (found == false)
        fatal(Errors::NoComponents, "no channels");
    if (G.total_channels == 0)
        fatal(Errors::NoComponents, "no channels");
}
