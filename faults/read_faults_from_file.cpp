
#include <cstddef>
#include <filesystem>
#include <fstream>

#include "read_faults_from_file.h"
#include "impose_faults_from_description.h"
#include "../errors/error_writing.h"
#include "../misc/misc_string_functions.h"
#include "../structs/structs_system.h"
#include "../globals.h"

using std::vector;
using std::string;
using std::size_t;

namespace fs = std::filesystem;


/**
This cpp contains functions concerning reading and imposing faults from a fault file.

The format of a fault file is something like:
-----
Fault file created at 11:09am 07 Apr 2026 for hypercube [n=5]

These faults were made by imposing the faults in the following descriptions:
 - <node,25%,0,"pr","*"> with intended number of faults 8
 - <channel,0%,5,"pr","pr"> with intended number of faults 5
The total numbers of faults set are:
 faulty nodes: 8
  - 8 processor faults
 faulty channels: 45
  - 45 processor-processor faults
Fault seeds: [2435365092,4043948059,3249365208,2116659304,918226801,3746117155,1035060498,2917517672]
Time to impose faults: 0 seconds

<NODE>
pr56,pr4,...
<CHANNEL>
pr2-11,pr3-7,pr3-11,...

Time to process and write faults: 0 seconds
<END OF FILE>
-----
When we read a fault file, we look for the prefixes:
 - "faulty nodes:"
 - "faulty channels:"
to get the numbers of faulty nodes and channels in the file. Then we look for the prefixes:
 - "<NODE>"
 - "<CHANNEL>"
and if the number of faulty nodes or channels is > 0 then the next line is read as a comma-separated list
of all the faulty nodes or channels, respectively. All other lines are ignored. Outside of the above,
anything can appear. It must be the case that the prefixes come in the following order:
 - "faulty nodes:"
 - "faulty channels:"
 - "<NODE>"
 - "<CHANNEL>"
and all prefixes have to be present. All node and channel ids that are read, e.g., pr54 and pr54-3, are
relative and converted into absolute ids.

Reading a fault file:
 - look_for_prefix()
 - check_token_size_and_node()
 - validate_node_fault()
 - validate_channel_fault()
 - get_number_of_faults_from_file()
 - get_actual_faults_from_file()
 - read_faults_from_file()
Fault management:
 - impose_list_of_faults()
 - remove_all_faults()
**/

/**
********************
Reading a fault file
********************
**/

size_t look_for_prefix(//supplied data:
                       const string&    line_string,
                       const string&    prefix)
/*
Check the stripped 'line_string' to see if it starts with 'prefix'. If not then 'string::npos' is returned;
if so then the location of the first char of 'line_string' after 'prefix' is returned. If 'prefix' is
empty then 'string::npos' is returned.
*/
{
    if (prefix.empty())
        return string::npos;
    if (line_string.size() < prefix.size())
        return string::npos;
    if (line_string.substr(0, prefix.size()) == prefix)
        return prefix.size();
    else
        return string::npos;
}

Errors check_token_size_and_node(// supplied data:
                                 const string&    token,
                                 const size_t     length,
                                 // computed data:
                                 size_t&          node_type)
/*
Checks that 'token', which will be something like pr34 or pr54-5, is not too short and doesn't contain
a bad short-name. The numeric 'node_type' of a node with a good short-name is returned (if there is an
error then this is treated as garbage; actually, equal to 0).

Errors:
 - Good, BadToken
*/
{
    bool found = false;
    node_type = 0;                                          // initialize in case of error

    if (token.size() < length)
        return Errors::BadToken;                            // 'token' too short
    for (size_t i = 0; i < System::num_node_types; ++i)     // check that 2-char prefix is legal short-name
    {
        if (token.substr(0, 2) == G.node_types[i].short_name)
        {
            found = true;
            node_type = i;
            break;
        }
    }
    if (found == false)
        return Errors::BadToken;                            // short-name of 'token' illegal
    return Errors::Good;
}

Errors validate_node_fault(//supplied data:
                           const string&    token,
                           // computed data:
                           size_t&          absolute_id)
/*
Validate that a 'token' encodes a legal node of the form, e.g., pr45, and translate to its absolute id
'absolute_id' (which is checked to see that it is within bounds). If there is an error then 'absolute_id'
is treated as garbage. Note that relative ids are encoded within tokens.

Errors:
 - Good, BadToken

Functions:
 - check_token_size_and_node            from here
*/
{
    Errors error;
    size_t node_type;
    size_t relative_id;

    error = check_token_size_and_node(token, 3, node_type);     // check token length and node short-name
    if (error != Errors::Good)
        return error;
    try
    {
        relative_id = std::stoull(token.substr(2));             // rel id is rest of string (non-empty)
        if (relative_id >= G.node_types[node_type].number)
            return Errors::BadToken;                            // relative id out of range
        absolute_id = G.node_types[node_type].first + relative_id;// convert to absolute id
        return Errors::Good;
    }
    catch (...)
    {
        return Errors::BadToken;                                // ill-formed absolute id
    }
}

Errors validate_channel_fault(//supplied data:
                              const vector<node>&    int_net,
                              const string&          token,
                              // computed data:
                              size_t&                absolute_id,
                              size_t&                the_port)
/*
Validate that a 'token' encodes a legal channel of the form, e.g., pr45-3, and translate to its absolute id
'absolute_id' and its port 'the_port'. If there is an error then 'absolute_id' and 'the_port' are treated as
garbage. Note that relative ids are encoded within tokens.

Errors:
 - Good, BadToken

Functions:
 - check_token_size_and_node            from here
*/
{
    Errors error;
    size_t node_type;
    size_t pos_hyphen;
    size_t number_of_ports;
    size_t relative_id;

    error = check_token_size_and_node(token, 5, node_type);     // check token length and node short-name
    if (error != Errors::Good)
        return error;                                           // 'token' too short or bad short-name
    pos_hyphen = token.find("-", 2);                            // look for hyphen
    if (pos_hyphen == string::npos)
        return Errors::BadToken;                                // no hypen in 'token'
    if (pos_hyphen < 3)
        return Errors::BadToken;                                // empty absolute id
    try
    {
        relative_id = stoull(token.substr(2, pos_hyphen - 2));  // relative id
        if (relative_id >= G.node_types[node_type].number)
            return Errors::BadToken;                            // out of bounds
        absolute_id = G.node_types[node_type].first + relative_id;// convert to absolute id
        if (pos_hyphen == token.size() - 1)
            return Errors::BadToken;                            // empty port number
        try
        {
            the_port = stoull(token.substr(pos_hyphen + 1));    // port number
            number_of_ports = int_net[absolute_id].node_get_num_ports();// # of ports
            if (the_port < number_of_ports)                     // check port number within range
                return Errors::Good;
            else
                return Errors::BadToken;                        // bad port number
        }
        catch (...)
        {
            return Errors::BadToken;                            // no valid port number
        }
    }
    catch (...)
    {
        return Errors::BadToken;                                // no valid relative id
    }
}

Errors get_number_of_faults_from_file(// supplied data:
                                      const string&    the_prefix,
                                      fs::path&        path_to_file,
                                      const string&    line_string,
                                      // supplied/computed data:
                                      size_t&          expected_number_of_faults)
/*
Look for 'the_prefix' which is:
 - "faultynodes:"
 - "faultychannels:"
in 'line_string' and get the number from rest of line. If 'error' != Good then
'expected_number_of_faults' = 0.

Errors:
 - Good, BadFaultNumber, FaultNumberNotFoundYet

Functions:
 - look_for_prefix          from here
 - warning                  from error_writing
*/
{
    expected_number_of_faults = 0;
    size_t a_size_t = look_for_prefix(line_string, the_prefix);
    if (a_size_t != string::npos)                       // if no prefix then do nothing and read next line
    {
        string rest_line_string = line_string.substr(a_size_t);// keep everything after the prefix
        try
        {
            expected_number_of_faults = std::stoull(rest_line_string);// get number of faulty components
        }
        catch (...)
        {
            warning(Errors::BadFaultNumber, path_to_file.string() + " re: " + the_prefix);
            return Errors::BadFaultNumber;
        }
        return Errors::Good;
    }
    return Errors::FaultNumberNotFoundYet;
}

void get_actual_faults_from_file(// supplied data:
                                 const vector<node>&    int_net,
                                 const string&          component,
                                 size_t                 expected_number_of_faults,
                                 string&                line_string,
                                 // supplied/computed data:
                                 int&                   mode,
                                 FaultBatch&            read_faults,
                                 // other stuff:
                                 std::ifstream&         a_fault_input_file)
/*
Read the faults from a file. 'line_string' is the just-read line from a file. 'component' is either
'<NODE>' or '<CHANNEL>'. If a prefix match is found then 'mode' is incremented. All faults found are added
to 'read_faults'.

Functions:
 - look_for_prefix                  from here
 - validate_node_fault              from here
 - validate_channel_fault           from here
 - warning                          from error_writing
*/
{
    Errors error;
    size_t a_size_t;
    string token;                                       // a token, e.g., pr34 or pr34-1
    size_t absolute_id;                                 // an abs. id
    size_t the_port;                                    // a port

    a_size_t = look_for_prefix(line_string, component); // check whether 'component' appears as a prefix
    if (a_size_t != string::npos)                       // if no prefix then do nothing and read next line
    {                                                   //  otherwise get the nodes or channels
        mode++;                                         // register 'component' found and move on
        if (expected_number_of_faults > 0)              // if no faulty components then do nothing and move on
        {
            getline(a_fault_input_file, line_string);   // read next line as the list of faulty components
            if (!line_string.empty() && line_string.back() == '\r')// remove Windows end-of-line (just in case)
                line_string.pop_back();                 // note that '\n' has been removed by getline
            token.clear();
            for (char ch : line_string)
            {
                if (ch == ',')
                {
                    if (component == "<NODE>")
                        error = validate_node_fault(token, absolute_id);
                    else
                        error = validate_channel_fault(int_net, token, absolute_id, the_port);
                    if (error == Errors::Good)
                    {
                        if (component == "<NODE>")
                            read_faults.nodes.push_back(absolute_id);// register faulty node
                        else
                        {
                            read_faults.channels.push_back(absolute_id);// register source node
                            read_faults.ports.push_back(the_port);// register port channel
                        }
                    }
                    else
                        warning(Errors::BadToken, token);
                    token.clear();
                }
                else
                    token = token + ch;                 // build 'token'
            }
            if (!token.empty())
            {
                if (component == "<NODE>")
                    error = validate_node_fault(token, absolute_id);
                else
                    error = validate_channel_fault(int_net, token, absolute_id, the_port);
                if (error == Errors::Good)
                {
                    if (component == "<NODE>")
                        read_faults.nodes.push_back(absolute_id);// register faulty node
                    else
                    {
                        read_faults.channels.push_back(absolute_id);// register source node
                        read_faults.ports.push_back(the_port);// register port channel
                    }
                }
                else
                    warning(Errors::BadToken, token);
            }
        }
    }
}

void read_faults_from_file(// supplied data:
                           const vector<node>&    int_net,
                           const string&          the_input_fault_file,
                           // computed data:
                           FaultBatch&            the_faults_read,
                           // other stuff:
                           std::ostream&          output)
/*
The input filename 'the_input_fault_file' is opened and the node, channel and link faults are read from it.
The faults will be written into:
 - 'the_faults_read.nodes'    = a list of abs-ids of all faulty nodes
 - 'the_faults_read.channels' = a list of abs-ids of sources of channel faults with
 - 'the_faults_read.ports'    = corresponding list of ports of same length so that together they detail
                                the faulty channels.
I need access to 'int_net' as I have to look to see the number of output ports at a node. After reading
from file, the faults read from file need to be double-checked for, e.g., repetitions.

I assume that 'the_input_fault_file' ends in '.txt'.I also assume that it lives in an existing folder
called 'fault_files' in the folder 'IN.name' in the current folder: I check this and ignore the fault
file if the folder or file don't exist as they should.

Functions:
 - strip_string                             from misc_string_functions
 - get_number_of_faults_from_file           from here
 - get_actual_faults_from_file              from here
 - warning                                  from error_writing
*/
{
    Errors error;
    int mode = 0;                               // mode = 0 means looking for 'faultynodes:'
    string line_string;                         // line from file
    string token;                               // fault string read from fault file

    the_faults_read = FaultBatch{};
    size_t expected_num_node_faults;
    size_t expected_num_channel_faults;

    fs::path path_to_file = fs::current_path() / "ACTUAL_NETWORKS" / fs::path(IN.name);
    std::cout << path_to_file.string() << '\n';
    if ((!fs::exists(path_to_file)) || (!fs::is_directory(path_to_file)))
    {
        warning(Errors::NoIntNetFolder, IN.name);
        return;
    }
    path_to_file = path_to_file / fs::path("fault_files");
    if ((!fs::exists(path_to_file)) || (!fs::is_directory(path_to_file)))
    {
        warning(Errors::NoFaultFolder, path_to_file.string());
        return;
    }
    path_to_file = path_to_file / the_input_fault_file;
    std::ifstream a_fault_input_file(path_to_file);// open 'path_to_file' for reading
    if (!a_fault_input_file)
    {
        warning(Errors::CantOpenFile, path_to_file.string());
        return;
    }

    output << "\nreading fault file '" << the_input_fault_file << "' ...\n";

    while (getline(a_fault_input_file, line_string))// read the (open) input file line by line
    {
        if (!line_string.empty() && line_string.back() == '\r')// remove Windows end-of-line (just in case)
            line_string.pop_back();
        string stripped_string = strip_string(line_string);// strip the line (ignore spaces, controls, ...)
        if (mode == 0)                          // looking for "faultynodes:"
        {
            error = get_number_of_faults_from_file("faultynodes:", path_to_file, stripped_string,
                                                   expected_num_node_faults);
            if (error == Errors::Good)
            {
                mode++;                         // now start looking for "faultychannels:" (mode 1)
                if (expected_num_node_faults > 0)
                    the_faults_read.nodes.reserve(expected_num_node_faults);
            }
            else if (error == Errors::BadFaultNumber)
                return;
        }
        else if (mode == 1)                     // looking for "faultychannels:"
        {
            error = get_number_of_faults_from_file("faultychannels:", path_to_file, stripped_string,
                                                   expected_num_channel_faults);
            if (error == Errors::Good)
            {
                mode++;                         // now start looking for "<NODE>" (mode 2)
                if (expected_num_node_faults > 0)
                {
                    the_faults_read.channels.reserve(expected_num_channel_faults);
                    the_faults_read.ports.reserve(expected_num_channel_faults);
                }
            }
            else if (error == Errors::BadFaultNumber)
                return;
        }
        else if (mode == 2)                     // looking for "<NODE>"
        {
            get_actual_faults_from_file(int_net, "<NODE>", expected_num_node_faults, stripped_string, mode,
                                        the_faults_read, a_fault_input_file);
        }
        else if (mode == 3)                     // looking for "<CHANNEL>"
        {
            get_actual_faults_from_file(int_net, "<CHANNEL>", expected_num_channel_faults, stripped_string,
                                        mode, the_faults_read, a_fault_input_file);
        }
        else
            break;
    }
    if (mode == 0)
        warning(Errors::MissingTag, "'faulty nodes:' in " + the_input_fault_file);
    else if (mode == 1)
        warning(Errors::MissingTag, "'faulty channels:' in " + the_input_fault_file);
    else if (mode == 2)
        warning(Errors::MissingTag, "'<NODE>' in " + the_input_fault_file);
    else if (mode == 3)
    {
        warning(Errors::MissingTag, "'<CHANNELS>:' in " + the_input_fault_file);
        the_faults_read.nodes.clear();
    }
    else
    {
        output << "read " << the_faults_read.nodes.size() << " faulty nodes from the fault file ("
            << expected_num_node_faults << " expected)\n" << "read " << the_faults_read.channels.size()
            << " faulty channels from the fault file (" << expected_num_channel_faults << " expected)\n";
    }
}

/**
****************
Fault management
****************
**/

void impose_list_of_faults(// supplied data:
                           vector<node>&        int_net,
                           const FaultBatch&    the_faults_read,
                           // supplied/computed data:
                           FaultBatch&          batch_of_faults)
/*
Impose the faults in the supplied lists in 'the_faults_read'. These faults might be a reimposition of
an already constructed list of faults or faults read from a file. It is assumed that the current node
and channel faults in the interconnection network are given by 'batch_of_faults' (with no repetitions).
On return, these registered lists of faults have been updated with the faults in 'the_faults_read' so
that there are no repetitions registered (this happens because when we make a node or channel of
'the_faults_read' faulty, it was because it was previously healthy so was not in 'batch_of_faults').

Functions:
 - test_node            from impose_faults_from_file.h
 - test_channel         from impose_faults_from_file.h
*/
{
    batch_of_faults.nodes.reserve(batch_of_faults.nodes.size() + the_faults_read.nodes.size());
    for (size_t i = 0; i < the_faults_read.nodes.size(); ++i)
    {
        if (test_node(int_net, the_faults_read.nodes[i]))             // note that no repetitions added
        {
            int_net[the_faults_read.nodes[i]].node_set_health(System::faulty);
            batch_of_faults.nodes.push_back(the_faults_read.nodes[i]);
        }
    }
    batch_of_faults.channels.reserve(batch_of_faults.channels.size() + the_faults_read.channels.size());
    batch_of_faults.ports.reserve(batch_of_faults.ports.size() + the_faults_read.ports.size());
    for (size_t i = 0; i < the_faults_read.channels.size(); ++i)
    {
        if (test_channel(int_net, the_faults_read.channels[i], the_faults_read.ports[i]))
        {
            int_net[the_faults_read.channels[i]].node_set_port_health(System::out, the_faults_read.ports[i],
                                                                      System::faulty);
            batch_of_faults.channels.push_back(the_faults_read.channels[i]);
            batch_of_faults.ports.push_back(the_faults_read.ports[i]);
        }
    }
}

void remove_all_faults(// supplied data:
                       vector<node>&    int_net)
/*
Removes all faults from 'int_net'. It doesn't bother to look for faults to remove but sets all nodes and
channels to be healthy.
*/
{
    for (size_t a_node = 0; a_node < G.total_nodes; a_node++)
    {
        int_net[a_node].node_set_health(System::healthy);
        size_t num_ports = int_net[a_node].node_get_num_ports();
        for (size_t a_port = 0; a_port < num_ports; a_port++)
        {
            int_net[a_node].node_set_port_health(System::out, a_port, System::healthy);
        }
    }
}
