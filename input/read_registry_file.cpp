
#include <map>
#include <unordered_set>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <unordered_set>
#include <filesystem>

#include "read_registry_file.h"
#include "data_structures_for_config.h"
#include "../misc/misc_string_functions.h"
#include "../errors/error_writing.h"

using std::string;
using std::vector;
using std::size_t;
using std::map;

/**
This cpp file contains all functions that read the registry:
 - get_reg_int_net() deals with lines registering an interconnection network and its routing algorithms
 - get_reg_traffic_pattern() deals with lines registering traffic pattern names
 - get_reg_note() deals with lines registering notes for interconnection networks.
The main function is:
 - read_int_net_data_from_registry()
which collects registered data in:
 - vector<RegIntNet>    int_nets_registered
 - vector<string>       traffic_patterns_registered
 - vector<RegNote>      notes_registered.
Note that any interconnection network needs to have at least one routing algorithm.

Interconnection network/routing algorithm registration functions:
 - check_int_net_name()
 - check_dimension_names()
 - check_associated_parameters()
 - check_routing_algorithms()
 - get_reg_int_net
Traffic pattern registration functions:
 - get_reg_traffic_pattern()
Note registration functions:
 - get_reg_note()
Read the registry:
 - read_int_net_data_from_registry()
**/

/**
****************************************************************
Interconnection network/routing algorithm registration functions
****************************************************************
**/

Errors check_int_net_name(// supplied data:
                          const string&                 int_net_name,
                          const vector<RegIntNet>&      int_nets_registered,
                          // supplied/computed data:
                          RegIntNet&                    registered_int_net)
/*
Check that 'int_net_name' is not empty and doesn't already exist. If successful then 'int_net_name' is
registered in 'registered_int_net.name'. If there is an error then nothing is registered.

Errors:
 - Good, EmptyIntNetName, RepeatedIntNetName
*/
{
    if (int_net_name.empty())
        return Errors::EmptyIntNetName;
    for (const auto& int_net : int_nets_registered)
    {
        if (int_net_name == int_net.name)
            return Errors::RepeatedIntNetName;
    }
    registered_int_net.name = int_net_name;
    return Errors::Good;
}

Errors check_dimension_names(// supplied data:
                             const string&    string_of_dimension_names,
                             // supplied/computed data:
                             RegIntNet&       registered_int_net)
/*
We split 'string_of_dimension_names' into a vector of strings. We check that they are all non-empty and
correctly formatted as either a normal dimension name, such as n, or as a composite dimension name, such
as m*h, where h appears previously and m doesn't. Note that correctly formatted means:
 - all names are non-empty (including both parts of a composite name)
 - a non-composite name has not appeared so far
 - the prefix of a composite name has not appeared so far
 - the suffix of a composite name has appeared so far.
If all is well, 'registered_int_net.parameters' contains all the validated dimension names as a vector of
strings. Otherwise, 'registered_int_net.parameters' is not touched.

Errors:
 - Good, EmptyDimensionName, ExistingDimensionName, BadlyFormedDimensionName

Functions:
 - split_string         from data_structures

*/
{
    vector<string> different_names;             // the proper names m or prefixes m of composite names m*h
    size_t pos;                                 // position of a substring in a string
    string the_name;                            // a dimension name n or a prefix m in composite names m*h
    string the_number;                          // a suffix h in m*h

    vector<string> dimension_names_as_strings = split_string(string_of_dimension_names, "-");

    for (const auto& dim : dimension_names_as_strings)// for every dimension name ...
    {
        if (dim.empty())                        // check that a name is non-empty
            return Errors::EmptyDimensionName;
        pos = dim.find('*');                    // look for composite names
        if (pos == std::string::npos)           // if a normal name ...
        {
            the_name = dim;
            if (std::find(different_names.begin(), different_names.end(), the_name) != different_names.end())
                return Errors::ExistingDimensionName;// check that 'the_name' doesn't already exist
        }
        else                                    // if a composite name ...
        {
            if (pos == dim.size() - 1)          // check that "*" is not the last char
                return Errors::BadlyFormedDimensionName;
            the_name = dim.substr(0, pos);
            the_number = dim.substr(pos + 1);
            if (the_name.empty() || the_number.empty())
                return Errors::EmptyDimensionName;// at least one of the names is empty
            if (std::find(different_names.begin(), different_names.end(), the_name) != different_names.end())
                return Errors::ExistingDimensionName;// 'the_name' already exists
            if (std::find(different_names.begin(), different_names.end(), the_number) == different_names.end())
                return Errors::ExistingDimensionName;// 'the_number' doesn't already exists
        }
        different_names.push_back(the_name);    // register the new name found
    }
    registered_int_net.parameters = dimension_names_as_strings;
    return Errors::Good;
}

Errors check_associated_parameters(// supplied data:
                                   const vector<string>&            params_vector,
                                   const vector<ValueAndTriple>&    values_and_triples,
                                   // computed data:
                                   RegIntNet&                       registered_int_net)
/*
Check the final three registry parameters for validity where 'params_vector' is s.t.:
 - item 0 corresponds to the dimension list (already dealt with)
 - item 1 should correspond to RegistryNetMachine
 - item 2 should correspond to RegistryNetType
 - item 3 should correspond to RegistryNetBuild.
If successful then values are registered in:
 - 'registered_int_net.machine_type'
 - 'registered_int_net.network_type'
 - 'registered_int_net.symmetry'.
Note that we assume that there are exactly 4 items in 'params_vector'.

Errors:
 - Good, NotRegAllowable
*/
{
    TripleString triple = {"<REGISTRY>", "net_machine", params_vector[1]};
    auto it1 = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                           [&](const ValueAndTriple& vt)
    {
        return vt.triple == triple;
    });
    if (it1 != values_and_triples.end())
        registered_int_net.net_machine = std::get<RegistryNetMachine>(it1->value);
    else
        return Errors::NotRegAllowable;

    triple = {"<REGISTRY>", "net_type", params_vector[2]};
    auto it2 = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                           [&](const ValueAndTriple& vt)
    {
        return vt.triple == triple;
    });
    if (it2 != values_and_triples.end())
        registered_int_net.net_type = std::get<RegistryNetType>(it2->value);
    else
        return Errors::NotRegAllowable;

    triple = {"<REGISTRY>", "net_build", params_vector[3]};
    auto it3 = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                           [&](const ValueAndTriple& vt)
    {
        return vt.triple == triple;
    });
    if (it3 != values_and_triples.end())
        registered_int_net.net_build = std::get<RegistryNetBuild>(it3->value);
    else
        return Errors::NotRegAllowable;

    return Errors::Good;
}

Errors check_routing_algorithms(//supplied data:
                                const string&    string_of_routing_algs,
                                // computed data:
                                RegIntNet&       registered_int_net)
/*
Check that the list of routing algorithms is non-empty and all routing algorithm names are non-empty and
distinct. If successful then the vector of routing algorithms is stored in 'registered_int_net.routing_algs',
otherwise nothing is stored.

Errors:
 - Good, NoRoutingAlgs, RepeatedRoutingAlg

Functions:
 - split_string         from data_structures
*/
{
    vector<string> routing_algorithms = split_string(string_of_routing_algs, ",");

    std::unordered_set<string> seen;
    for (const auto& alg : routing_algorithms)  // check routing alg. names non-empty and distinct
    {
        if (alg.empty())
            return Errors::NoRoutingAlgs;
        if (!seen.insert(alg).second)
            return Errors::RepeatedRoutingAlg;
    }
    registered_int_net.routing_algs = routing_algorithms;
    return Errors::Good;
}

Errors get_reg_int_net(// supplied data:
                       const string&                    line_from_a_file,
                       const vector<ValueAndTriple>&    values_and_triples,
                       // computed/supplied data:
                       vector<RegIntNet>&               int_nets_registered)
/*
This functions handles a check that a line registers an interconnection network. The struct
'registered_int_net' is gradually built from 'line_from_a_file' within this function and added to
'int_nets_registered' if all is well. After any call to this function, the list of interconnection
networks 'int_nets_registered' is entirely valid and requires no further validation.

Errors:
 - Good, MissingRightBookend, MissingLeftBookend, EmptyIntNetName, RepeatedIntNetName, WrongNumParameters,
   EmptyDimensionName, ExistingDimensionName, BadlyFormedDimensionName, NotRegAllowable, NoRoutingAlgs,
   RepeatedRoutingAlg

Functions:
 - look_for_delimiters                  from data_structures
 - check_int_net_name                   from here
 - check_dimension_names                from here
 - check_associated_parameters          from here
 - check_routing_algorithms             from here
*/
{
    Errors error;
    RegIntNet registered_int_net;       // the struct to hold the int. net. entry
    size_t pos_left;
    size_t pos_right;

    // 'line_string' expected as: int-net-name[dimensions, p1, p2, p3]<routing-algorithms>

    error = look_for_delimiters(line_from_a_file, "[", "]", pos_left, pos_right);
    if (error != Errors::Good)
        return error;

    string int_net_name = line_from_a_file.substr(0, pos_left);
    error = check_int_net_name(int_net_name, int_nets_registered, registered_int_net);
    if (error != Errors::Good)
        return error;

    string string_of_int_net_params = line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1);
    vector<string> params_vector = split_string(string_of_int_net_params, ",");
    if (params_vector.size() != 4)
        return Errors::WrongNumParameters;
    error = check_dimension_names(params_vector[0], registered_int_net);
    if (error != Errors::Good)
        return error;
    error = check_associated_parameters(params_vector, values_and_triples, registered_int_net);
    if (error != Errors::Good)
        return error;

    if (pos_right == line_from_a_file.size() - 1)
        return Errors::NoRoutingAlgs;
    string string_of_routing_algs = line_from_a_file.substr(pos_right + 1);
    error = look_for_delimiters(string_of_routing_algs, "<", ">", pos_left, pos_right);
    if (error != Errors::Good)
        return error;
    string_of_routing_algs = string_of_routing_algs.substr(pos_left + 1, pos_right - pos_left - 1);
    error = check_routing_algorithms(string_of_routing_algs, registered_int_net);
    if (error != Errors::Good)
        return error;

    int_nets_registered.push_back(registered_int_net);
    return Errors::Good;
}

/**
**************************************
Traffic pattern registration functions
**************************************
**/

Errors get_reg_traffic_pattern(// supplied data:
                               const string&      stripped_string,
                               // supplied/computed data:
                               vector<string>&    traffic_patterns_registered)
/*
Split a string into traffic pattern names and check to see that they are non-empty and distinct with each
other and with existing traffic pattern names in 'traffic_patterns_registered'. All valid, unseen traffic
pattern names are registered in 'traffic_patterns_registered'. After any call to this function, the list
of traffic patterns 'traffic_patterns_registered' is entirely valid and requires no further validation.

Errors:
 - Good, EmptyTPname, InvalidTPname

Functions
 - split_string         from data_structures
*/
{
    Errors error = Errors::Good;
    std::unordered_set<string> seen(traffic_patterns_registered.begin(), traffic_patterns_registered.end());

    vector<string> tp_names = split_string(stripped_string, ",");
    for (const auto& tp : tp_names)
    {
        if (tp.empty())
        {
            error = Errors::EmptyTPname;
            continue;
        }
        if (!seen.insert(tp).second)
        {
            error = Errors::InvalidTPname;
            continue;
        }
        traffic_patterns_registered.push_back(tp);
    }
    return error;
}

/**
***************************
Note registration functions
***************************
**/

Errors get_reg_note(// supplied data:
                    const string&       line_from_a_file,
                    // computed/supplied data:
                    vector<RegNote>&    notes_registered)
/*
Read a possible note and its supposed interconnection network. Strip the interconnection network name and
trim the note of leading/trailing spaces. If the name is non-empty, store the name and the note in
'notes_registered'. After config has been read:
 - notes for unregistered interconnection networks need to be dispensed with
 - notes for the same interconnection network need to be amalgamated
 - interconnection networks with no note need to have an empty note associated.

Errors:
 - Good, InvalidNote, EmptyIntNetForNote

Functions:
 - strip_string         from data_structures
*/
{
    size_t pos = line_from_a_file.find('=');    // assume string is of the form "prefix = suffix"
    if (pos == string::npos)
        return Errors::InvalidNote;

    string prefix = line_from_a_file.substr(0, pos);
    prefix = strip_string(prefix);              // strip prefix
    if (prefix.empty())
        return Errors::EmptyIntNetForNote;

    ++pos;                                      // position after '='
    while (pos < line_from_a_file.size())       // skip leading spaces/controls
    {
        unsigned char ch = (unsigned char)line_from_a_file[pos];
        if (std::iscntrl(ch) || std::isspace(ch))
            ++pos;
        else
            break;
    }
    string suffix = "";
    if (pos < line_from_a_file.size())
    {
        suffix = line_from_a_file.substr(pos);

        pos = suffix.size();                    // skip trailing spaces/controls
        while (pos > 0)
        {
            --pos;
            unsigned char ch = (unsigned char)suffix[pos];
            if (!std::iscntrl(ch) && !std::isspace(ch))
                break;
        }
        suffix = suffix.substr(0, pos + 1);
    }

    RegNote a_note;
    a_note.name = prefix;
    a_note.note = suffix;
    notes_registered.push_back(a_note);
    return Errors::Good;
}

/**
*****************
Read the registry
*****************
**/

void read_int_net_data_from_registry(// supplied data:
                                     const string&                    input_file_name,
                                     const vector<ValueAndTriple>&    values_and_triples,
                                     // computed data:
                                     TheRegistry&                     the_registry,
                                     string&                          pre_registry,
                                     string&                          post_registry,
                                     // other stuff:
                                     std::ostream&                    output)
/*
Read the registry line by line and store the contents in 'the_registry' via:
 - 'int_nets_registered'
 - 'traffic_patterns_registered'
 - 'notes_registered'.
We also return 'pre_registry' and 'post_registry' as the strings before the first tag and after the eof (so
that we can tidy up the registry later when validating, if required).

As we read, we echo the stripped line from the registry to the screen, with an error message if things are
not ok, apart from notes where: we echo a stripped prefix and an unstripped note, if the note is valid; and
we echo the entire unstripped line from registry, otherwise (with a warning message). Note that if the input
file does not open and close cleanly and if the final line read is not "<ENDOFFILE>" then we interpret this
as a fatal error and terminate.

After the registry has been read, we will validate so that:
 - notes for unregistered interconnection networks have been dispensed with
 - notes for the same interconnection network have been amalgamated
 - interconnection networks with no note have had an empty note associated
 - there is at least one valid interconnection network
 - there is at least one valid traffic pattern.

Functions:
 - fatal                            from error_writing
 - strip_string                     from data_structures
 - get_reg_int_net                  from here
 - get_reg_traffic_pattern          from here
 - get_reg_note                     from here
*/
{
    Errors error;

    // open registry

    std::ifstream the_file(input_file_name);    // input file stream 'input_file_name'
    if (!the_file)                              // if there is something wrong then terminate
        fatal(Errors::ProblematicFile, input_file_name);
    output << "\n'" << input_file_name << "' has been opened\n";

    // read registry

    the_registry = TheRegistry{};

    output << "\nreading registry line by line ...\n";
    string line_string;                         // a line from 'input_file_name'
    ReadMode read_mode = ReadMode::no_read;
    while (std::getline(the_file, line_string)) // read the input file line by line
    {
        // strip the line

        string stripped_string = strip_string(line_string);// strip the string
        if ((stripped_string.empty()) && (read_mode != ReadMode::no_read)
            && (read_mode != ReadMode::end_of_file))// keep empty lines before start and after EOF
            continue;

        // check for change in read mode

        if ((stripped_string == "<INTERCONNECTIONNETWORK/ROUTINGALGORITHM>")
            && (read_mode != ReadMode::end_of_file))
        {
            read_mode = ReadMode::int_net;
            output << "\n<INTERCONNECTIONNETWORK/ROUTINGALGORITHM>\n";
        }
        else if ((stripped_string == "<TRAFFICPATTERN>")
                 && (read_mode != ReadMode::end_of_file))
        {
            read_mode = ReadMode::traffic_pattern;
            output << "\n<TRAFFICPATTERN>\n";
        }
        else if ((stripped_string == "<NOTES>")
                 && (read_mode != ReadMode::end_of_file))
        {
            output << "\n<NOTES>\n";
            read_mode = ReadMode::note;
        }
        else if ((stripped_string == "<ENDOFFILE>")
                 && (read_mode != ReadMode::end_of_file))
        {
            output << "\n<ENDOFFILE>\n";
            read_mode = ReadMode::end_of_file;
        }
        else
        {
            // check for new entry

            if (read_mode == ReadMode::int_net)
            {
                error = get_reg_int_net(stripped_string, values_and_triples,
                                        the_registry.int_nets_registered);
                output << stripped_string;
                if (error != Errors::Good)
                    output << short_warning(error);
                output << "\n";
            }
            else if (read_mode == ReadMode::traffic_pattern)
            {
                error = get_reg_traffic_pattern(stripped_string, the_registry.traffic_patterns_registered);
                output << stripped_string;
                if (error != Errors::Good)
                    output << short_warning(error);
                output << "\n";
            }
            else if (read_mode == ReadMode::note)
            {
                error = get_reg_note(line_string, the_registry.notes_registered);
                if (error == Errors::Good)
                    output << the_registry.notes_registered.back().name << " = "
                    << the_registry.notes_registered.back().note << "\n";
                else
                    output << line_string << short_warning(error);
            }
            else if (read_mode == ReadMode::no_read)
                pre_registry += line_string + "\n";
            else if (read_mode == ReadMode::end_of_file)
                post_registry += line_string + "\n";
        }
    }

    if (the_file.bad())
        fatal(Errors::ProblematicFile, "I/O error while reading file");
    if (the_file.fail() && !the_file.eof())
        fatal(Errors::ProblematicFile, "error while reading file");
    if (read_mode != ReadMode::end_of_file)
        fatal(Errors::NotViaEOF);
    output << "\n... '" << input_file_name << "' has been read, processed and closed\n";
}
