
#include <algorithm>
#include <chrono>
#include <optional>
#include "impose_faults_from_description.h"
#include "../globals.h"
#include "../construction/int_net_construction.h"

using std::vector;
using std::string;
using std::size_t;
using std::pair;
using std::array;

/**
This cpp contains functions involving imposing faults from config fault descriptions.

A string entry in config for a fault description is of the form:
 - ["node", node-type]<x> or <x\%> - x or x% faulty nodes of node type node-type, e.g., "pr", "sw", "*"
   where "*" denotes all nodes
 - ["channel", node-type1, node-type2]<x> or <x\%> - x or x% faulty channels of channel type <node-type1,
   node-type2>
 - ["link", node-type1, node-type2]<x> or <x\%> - x or x% faulty links of paired channels of type
   <node-type1, node-type2> and <node-type2, node-type1>.
If node-type1 or node-type2 is omitted or not in {"pr", "se", "sw", "os", "*"} then the node-type is
registered as "*". For channel or link faults where one node-type is omitted, "*" is assumed as the second
node-type. Note that it only makes sense to set links as faulty when the interconnection network is a full
duplex undirected graph.

The data from a valid fault description from config is saved as a struct FaultDescription:
 - name                     = string "node", "channel" or "link"
 - percentage               = 0 <= double < 100
 - raw_count                = size_t >= 0
 - source_node_type         = node type in {"pr", "se", "sw", "os", "*"}
 - dest_node_type           = node type in {"pr", "se", "sw", "os", "*"}
 - number_of_faults         = aimed for number of faults to be set.
Exactly one of percentage and raw_count is 0 in a valid fault description. If we have a node description
then 'dest_node_type' = "*". All valid fault descriptions are stored in 'FAU.fault_description'.

In general, fault imposition works as follows. Denote the faults in 'FAU.fault_description' set by a
call to 'impose_faults_description()' by F and add a ' for such faults imposed by a subsequent call to
'impose_faults_description()'. So, if we called 'impose_faults_description()' 4 times then we would
generate the sets of faults F, F', F'' and F'''. We cycle through the fault files per traffic pattern
run. Suppose that there are 3 fault files and 10 traffic pattern runs.
If 'FAU.regenerate' = true then:
 - for tp 1, we impose F and then the faults in fault file 1
 - for tp 2, we impose F' and then the faults in fault file 2
 - for tp 3, we impose F'' and then the faults in fault file 3
 - for tp 4, we impose F''' and then the faults in fault file 1 and so on.
If 'FAU.regenerate' = false then:
 - for tp 1, we impose F and then the faults in fault file 1
 - for tp 2, we impose F and then the faults in fault file 2
 - for tp 3, we impose F and then the faults in fault file 3
 - for tp 4, we impose F and then the faults in fault file 1 and so on.

Miscellaneous functions:
 - get_index_from_short_name()
 - reserve_space_for_fault_batch()
Functions computing actual numbers of faults:
 - get_number_of_faults_expected()
Functions involving node faults:
 - guess_a_possible_source()
 - register_bad_guess()
 - test_node()
 - register_new_faulty_node()
Functions involving channel faults:
 - guess_a_possible_channel()
 - test_channel()
 - test_channel_plus_target()
 - register_new_faulty_channel()
Impose faults from the fault descriptions:
 - impose_faults_description()
**/

/**
***********************
Miscellaneous functions
***********************
**/

size_t get_index_from_short_name(// supplied data:
                                 const string&    short_name)
/*
Given a short-name of a node type, return the index of the node type.
*/
{
    auto it = std::find_if(G.node_types.begin(), G.node_types.end(), [&](const NodeType& nt)
                           {return nt.short_name == short_name;});
    if (it != G.node_types.end())
        return std::distance(G.node_types.begin(), it);
    else
        coding(Errors::ShortNameMismatch);
    return 0;               // we never get to here!
}

void reserve_space_for_fault_batch(// supplied data:
                                   const FaultDescription&    fd,
                                   // supplied/computed data:
                                   FaultBatch&                batch_of_faults)
/*
Reserves space for the various lists of faults in a fault description. Note that we need to reserve space
for twice the number of links given as each faulty link means the setting of two faulty channels. We
assume that:
 - 'fd.name' in {"node", "channel", "link"}
*/
{
    if (fd.name == "node")
        batch_of_faults.nodes.reserve(fd.number_of_faults);
    else if (fd.name == "channel")
    {
        batch_of_faults.channels.reserve(fd.number_of_faults);
        batch_of_faults.ports.reserve(fd.number_of_faults);
    }
    else if (fd.name == "link")
    {
        batch_of_faults.channels.reserve(2 * fd.number_of_faults);
        batch_of_faults.ports.reserve(2 * fd.number_of_faults);
    }
}

/**
********************************************
Functions computing actual numbers of faults
********************************************
**/

Errors get_number_of_faults_expected(// supplied/computed data:
                                     FaultDescription&    fd)
/*
Computes how many actual faults should be set. When we are dealing with links, we guess the total number
of links in the int. net. as #-channels / 2, i.e., we assume an underlying undirected graph. All
percentage fractions are truncated. The actual number of faults expected is loaded into
'fd.number_of_faults'. We assume that:
 - at least one of 'fd.raw_count' and 'fd.percentage' is non-zero
 - 'fd.source_node_type' and 'fd.dest_node_type' in {"pr", "se", "sw", "os", "*"}
 - 'fd.name' in {"node", "channel", "link"}

Errors:
 - Good, NoComponents, BadCoding

Functions:
 - nn_to_c          from struct_file
*/
{
    size_t total;
    size_t channel_type_index;

    // deal with node faults

    if (fd.name == "node")
    {
        if (fd.source_node_type != "*")             // calculate total number of nodes of given type
        {
            size_t index = get_index_from_short_name(fd.source_node_type);
            total = G.node_types[index].number;
        }
        else
            total = G.total_nodes;
        if (total == 0)                             // there are no nodes to make faulty
            return Errors::NoComponents;
        if (fd.raw_count == 0)                      // calc/register total number of faulty nodes
            fd.number_of_faults = static_cast<size_t>(fd.percentage * total / 100);
        else
            fd.number_of_faults = fd.raw_count;
        return Errors::Good;
    }

    // deal with channel/link faults

    else if ((fd.name == "channel") || (fd.name == "link"))
    {
        if ((fd.source_node_type != "*") && (fd.dest_node_type != "*"))
        {                                               // calc total num channels/links of given type
            size_t index1 = get_index_from_short_name(fd.source_node_type);
            size_t index2 = get_index_from_short_name(fd.dest_node_type);
            channel_type_index = nn_to_c(index1, index2);
            total = G.channel_types[channel_type_index].number;
        }
        else if ((fd.source_node_type == "*") && (fd.dest_node_type != "*"))
        {
            total = 0;
            size_t index2 = get_index_from_short_name(fd.dest_node_type);
            for (size_t i = 0; i < System::num_node_types; ++i)
            {
                channel_type_index = nn_to_c(i, index2);
                total += G.channel_types[channel_type_index].number;
            }
        }
        else if ((fd.source_node_type != "*") && (fd.dest_node_type == "*"))
        {
            total = 0;
            size_t index1 = get_index_from_short_name(fd.source_node_type);
            for (size_t i = 0; i < System::num_node_types; ++i)
            {
                channel_type_index = nn_to_c(index1, i);
                total += G.channel_types[channel_type_index].number;
            }
        }
        else
            total = G.total_channels;
        if (fd.name == "link")
            total = static_cast<size_t>(total / 2);                     // guessed number of links
        if (total == 0)                                                 // there are no channels/links to make faulty
            return Errors::NoComponents;
        if (fd.raw_count == 0)                                          // calc/register total number of faulty channels
            fd.number_of_faults = static_cast<size_t>(fd.percentage * total / 100);
        else
            fd.number_of_faults = fd.raw_count;
        return Errors::Good;
    }
    return Errors::BadCoding;                                           // this shouldn't happen!
}

/**
*******************************
Functions involving node faults
*******************************
**/

size_t guess_a_possible_source(// supplied data:
                               const string&    node_type)
/*
Guesses a source (terminal) node of a given type supplied as a short-name. We assume that:
 - 'node_type' in {"pr", "se", "*"}

Functions:
 - RNG_fault.getSizeT           from random_gen
*/
{
    size_t possible_source;
    if (node_type == "*")
        possible_source = RNG_fault.getSizeT(0, G.num_terms - 1);
    else
    {
        size_t index = get_index_from_short_name(node_type);
        possible_source = RNG_fault.getSizeT(G.node_types[index].first, G.node_types[index].last - 1);
    }
    return possible_source;
}

void register_bad_guess(// supplied data:
                        const size_t     num_faults_set_so_far,
                        const size_t     num_faults,
                        // supplied/computed data:
                        size_t&          bad_guesses,
                        // other stuff:
                        std::ostream&    output)
/*
Registers a bad guess (and displays current progress if 'FAU.show_generation' = true).
*/
{
    bad_guesses++;
    if (FAU.show_generation)
        output << "a bad guess has been made while setting nodes: so far, " << num_faults_set_so_far
            << " faulty nodes/channels have been set from an intended " << num_faults << " with "
            << bad_guesses << " bad guesses made from a limit of " << FAU.limit_guesses << "\n";
}

bool test_node(// supplied data:
               vector<node>&    int_net,
               const size_t     source)
/*
Returns true if a node is healthy and false if it is faulty.
*/
{
    if (int_net[source].node_get_health() == System::healthy)
        return true;
    else
        return false;
}

void register_new_faulty_node(// supplied data:
                              vector<node>&    int_net,
                              const size_t     source,
                              // supplied/computed data:
                              FaultBatch&      batch_of_faults,
                              size_t&          num_faults_set_so_far)
/*
Register a known healthy node as faulty. Note that when a healthy node is made faulty, all of its
out-ports are too. Also, 'num_faults_set_so_far' = the number of faulty nodes set so far is incremented.
Note that adjacent healthy channels (but not already faulty ones) are made faulty too but that we
don't count these channels. When all of the intended faulty nodes and channels have been set, we'll
be able to calculate the exact number of faulty nodes and channels. The upshot is that this function
does not replicate faulty nodes or channels. We assume that:
 - 'source' is a healthy node on input
*/
{
    int_net[source].node_set_health(System::faulty);
    batch_of_faults.nodes.push_back(source);
    num_faults_set_so_far++;

    // make out-ports faulty too

    size_t num_ports = int_net[source].node_get_num_ports();
    for (size_t i = 0; i < num_ports; ++i)
    {
        if (int_net[source].node_get_port_health(System::out, i) == System::healthy)
        {                                                           // only set healthy channels as faulty
            int_net[source].node_set_port_health(System::out, i, System::faulty);// make channel faulty
            batch_of_faults.channels.push_back(source); // register faulty channel
            batch_of_faults.ports.push_back(i);
        }
    }
}

/**
**********************************
Functions involving channel faults
**********************************
**/

size_t guess_a_possible_channel(// supplied data:
                                const vector<node>&    int_net,
                                const size_t           source)
/*
Guess a possible port from a given node.

Functions:
 - RNG_fault.getSizeT           from random_gen
*/
{
    size_t num_ports = int_net[source].node_get_num_ports();
    size_t a_port = RNG_fault.getSizeT(0, num_ports - 1);
    return a_port;
}

bool test_channel(// supplied data:
                  const vector<node>&    int_net,
                  const size_t           source,
                  const size_t           source_port)
/*
Return true if a channel is healthy and false if it is faulty.
*/
{
    if (int_net[source].node_get_port_health(System::out, source_port) == System::healthy)
        return true;
    else
        return false;
}

bool test_channel_plus_target(// supplied data:
                              const vector<node>&    int_net,
                              const size_t           source,
                              const size_t           source_port,
                              const string&          target_node_type)
/*
Returns true if a channel is healthy and has a target of the required type (short-name), and false
otherwise. We assume that:
 - 'target_node_type' in {"pr", "se", "sw", "os", "*"}

Functions:
 - test_channel         from here
*/
{
    if (test_channel(int_net, source, source_port))
    {
        if (target_node_type != "*")
        {
            size_t target = int_net[source].node_get_port_node(System::out, source_port);
            size_t index = get_index_from_short_name(target_node_type);
            if (int_net[target].node_get_type() != index)// check target type
                return false;
            else
                return true;
        }
        else
            return true;
    }
    else
        return false;
}

void register_new_faulty_channel(// supplied data:
                            vector<node>&    int_net,
                            const size_t     source,
                            const size_t     source_port,
                            // supplied/computed data:
                            FaultBatch&      batch_of_faults,
                            size_t&          num_faults_set_so_far)
/*
Register a known healthy channel as faulty. Also, 'num_faults_set_so_far' = the number of faulty channels
set so far is incremented. The upshot is that this function does not replicate faulty channels. We assume
that:
 - 'source'/'source_port' is a healthy channel on input.
*/
{
    int_net[source].node_set_port_health(System::out, source_port, System::faulty);
    batch_of_faults.channels.push_back(source);
    batch_of_faults.ports.push_back(source_port);
    num_faults_set_so_far++;
}

/**
*****************************************
Impose faults from the fault descriptions
*****************************************
**/

void impose_faults_description(// supplied data:
                               vector<node>&    int_net,
                               // computed data:
                               FaultBatch&      batch_of_faults,
                               // other stuff:
                               std::ostream&    output)
/*
We impose faults from the fault descriptions in 'FAU.fault_description' here before moving on to a
fault file later. Each fault description has faults stored in:
 - 'batch_of_faults_per_desc.nodes'
 - 'batch_of_faults_per_desc.channels'
 - 'batch_of_faults_per_desc.ports'
before these faults are added to
 - 'batch_of_faults.nodes'
 - 'batch_of_faults.channels'
 - 'batch_of_faults.ports'
and returned. Whenever this function is called, we assume (without checking) that there are currently no
faults set. Faults are set in order of fault descriptions and we do not reset already faulty nodes or
channels as faulty, with 'FAU.limit_guesses' providing a limit as to when we should stop trying. So, we
might end up by setting a different number of faulty nodes and channels than intended. The faulty nodes
and channels in 'batch_of_faults.nodes', 'batch_of_faults.channels' and 'batch_of_faults.ports' contain
no repetitions and so provide an accurate count of the number of faulty nodes and channels. Note that
when we make a node faulty, all of its (healthy) out-ports are made faulty too.

We check here when we make nodes and channels faulty that:
 - there always exists a node/channel/link of the required type in an individual fault description
   (otherwise the description is ignored).
Note that the total number of faults trying to be set might be more than the total number of components
or we might be trying to set 0 faults.

Functions:
 - get_number_of_faults_expected            from here
 - warning                                  from error_writing
 - reserve_space_for_fault_batch            from here
 - guess_a_possible_source                  from here
 - test_node                                from here
 - register_new_faulty_node                 from here
 - guess_a_possible_channel                 from here
 - test_channel_plus_target                 from here
 - register_new_faulty_channel              from here
 - ports_between_nodes                      from int_net_construction
 - register_bad_guess                       from here
 - test_channel                             from here
*/
{
    string error_string;                    // a string to feature in error messages
    Errors error;
    size_t bad_guesses;                     // bad guesses made for an individual fd
    size_t num_faults_set_so_far;           // number of faults set so far in an individual fd
    size_t possible_source;                 // a possible source node
    size_t source_port;                     // a port at the source
    size_t possible_target;                 // a possible neighbour of a node
    size_t target_port;                     // a target port
    pair<pair<std::optional<size_t>, std::optional<size_t>>,pair<std::optional<size_t>,
                std::optional<size_t>>> pairs_ports;// pairs of ports for a non-symmetric build

    FaultBatch batch_of_faults_per_desc;    // faults set per fault description
    size_t total_intended_faults = 0;       // total number of intended faults

    // work through each fault description in turn

    output << "\nsetting faults from fault descriptions ...\n";

    batch_of_faults = FaultBatch{};         // overall lists of faulty nodes, channels and ports

    for (auto& fd : FAU.fault_description)  // for each individual fault description
    {                                       //  (note that we alter them so no const)
        batch_of_faults_per_desc = FaultBatch{};// clear all faults per fault description

        // check whether the fault description is redundant

        error = get_number_of_faults_expected(fd);  // get the number of faults to be set
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(System::dp_precision) << fd.percentage;
        error_string = "[" + fd.name + "," + ss.str() + "%,"
                        + std::to_string(fd.raw_count) + "," + fd.source_node_type + ","
                        + fd.dest_node_type + "]";
        if (error == Errors::NoComponents)  // no components to be made faulty
        {
            warning(Errors::EmptyFaultDesc, error_string);
            continue;
        }
        if (fd.number_of_faults == 0)       // no faults to be set
        {
            warning(Errors::EmptyFaultDesc, error_string);
            continue;
        }
        output << "\nsetting faults for " << error_string << " ...\n";

        // set the faults (# of faults to be set is > 0 and there are healthy components)

        reserve_space_for_fault_batch(fd, batch_of_faults_per_desc);// reserve space for upcoming faults

        if ((fd.name == "node") || (fd.name == "channel"))// update total number of intended faults
            total_intended_faults = total_intended_faults + fd.number_of_faults;
        else
            total_intended_faults = total_intended_faults + (2 * fd.number_of_faults);// links
        bad_guesses = 0;                            // this is where fault-setting starts
        num_faults_set_so_far = 0;

        while ((num_faults_set_so_far < fd.number_of_faults) && (bad_guesses < FAU.limit_guesses))
        {                             // set the faults (note that we time-out after too many bad guesses)
            possible_source = guess_a_possible_source(fd.source_node_type);// guess source of correct type
            if (test_node(int_net, possible_source))// test whether node is already faulty (= false)
            {
                if (fd.name == "node")              // we're setting faulty nodes
                    register_new_faulty_node(int_net, possible_source, batch_of_faults_per_desc,
                                             num_faults_set_so_far);
                else                                // we're setting faulty channels or links
                {
                    source_port = guess_a_possible_channel(int_net, possible_source);// guess a possible port
                    if (test_channel_plus_target(int_net, possible_source, source_port, fd.dest_node_type))
                    {                               // test if healthy channel of correct type
                        if (fd.name == "channel")   // we're setting faulty channels
                            register_new_faulty_channel(int_net, possible_source, source_port,
                                batch_of_faults_per_desc, num_faults_set_so_far);// register channel - move on
                        else                        // we're setting faulty links
                        {
                            possible_target = int_net[possible_source].node_get_port_node(System::out,
                                                                                          source_port);
                            if (RG.net_build == RegistryNetBuild::symmetric)// if symmetric build
                                target_port = source_port;
                            else                    // otherwise non-symmetric build
                            {
                                pairs_ports = ports_between_nodes(int_net, possible_source, possible_target);
                                if (!pairs_ports.second.first && !pairs_ports.second.second)
                                {                   // if no back channel
                                    register_bad_guess(num_faults_set_so_far, fd.number_of_faults,
                                                       bad_guesses, output);// register bad guess
                                    continue;
                                }
                                else                // otherwise get the target port
                                    target_port = *pairs_ports.second.first;
                            }
                            if (test_channel(int_net, possible_target, target_port))// test
                            {
                                register_new_faulty_channel(int_net, possible_source, source_port,
                                                            batch_of_faults_per_desc, num_faults_set_so_far);
                                register_new_faulty_channel(int_net, possible_target, target_port,
                                                            batch_of_faults_per_desc, num_faults_set_so_far);
                            }
                            else                    // register link - move on
                                register_bad_guess(num_faults_set_so_far, fd.number_of_faults,
                                                   bad_guesses, output);// register bad guess
                        }
                    }
                    else
                        register_bad_guess(num_faults_set_so_far, fd.number_of_faults,
                                           bad_guesses, output);// register bad guess
                }
            }
            else                                    // if the guessed node is faulty
                register_bad_guess(num_faults_set_so_far, fd.number_of_faults, bad_guesses, output);
        }                                           // fault description faults set and remembered in lists

        // feed back from the faults just generated and register them in global list

        output << "successfully set " << num_faults_set_so_far << " faults from an intended number of "
               << fd.number_of_faults << "\n";

        batch_of_faults.nodes.reserve(batch_of_faults.nodes.size() + batch_of_faults_per_desc.nodes.size());
        batch_of_faults.channels.reserve(batch_of_faults.channels.size()
                                         + batch_of_faults_per_desc.channels.size());
        batch_of_faults.ports.reserve(batch_of_faults.ports.size() + batch_of_faults_per_desc.ports.size());
        batch_of_faults.nodes.insert(batch_of_faults.nodes.end(), batch_of_faults_per_desc.nodes.begin(),
                                       batch_of_faults_per_desc.nodes.end());
        batch_of_faults.channels.insert(batch_of_faults.channels.end(),
                                        batch_of_faults_per_desc.channels.begin(),
                                        batch_of_faults_per_desc.channels.end());
        batch_of_faults.ports.insert(batch_of_faults.ports.end(), batch_of_faults_per_desc.ports.begin(),
                                       batch_of_faults_per_desc.ports.end());
    }

    output << "\nfinished setting faults from fault descriptions:\n";
    output << " - ";
    output << batch_of_faults.nodes.size() << " node faults and " << batch_of_faults.channels.size()
           << " channel faults have been set via fault descriptions ";
    output<< "from an intended number of " << total_intended_faults << " individual faults "
          << "(remember: setting a faulty node also makes each incident output channel faulty)\n";
}
