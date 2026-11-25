
#include <filesystem>

#include "validation.h"
#include "read_registry_file.h"
#include "../globals.h"
#include "../errors/error_writing.h"
#include "../structs/structs_system.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp file contains all function relating to the validation of the read config and the read registry:
 - tidy_up_registry()               tidies up a read registry so that it can be rewritten in this tidier
                                    form by rewrite_the_registry() if required
 - expand_named_list()              expands the parameter list of the given interconnection network and
                                    validates that all parameters and their names have been properly entered
 - validate_core_int_net_data()     validates that the interconnection network entered exists in the registry
 - validate_traffic_patterns()      validates traffic patterns entered and expands and batches them
 - validate_fault_descriptions()    validates all fault descriptions entered.

Validate the core interconnection network data
 - tidy_up_registry()
 - expand_named_list()
 - validate_core_int_net_data()
Validate traffic patterns
 - get_a_source_or_dest_per_type()
 - get_a_source_or_dest()
 - validate_traffic_patterns()
Validate fault descriptions:
 - get_number_of_node_type()
 - get_number_of_channel_type()
 - validate_fault_descriptions()
Rewrite the registry:
 - rewrite_the_registry()
**/

/**
**********************************************
Validate the core interconnection network data
**********************************************
**/

void tidy_up_registry(// supplied/computed data:
                      TheRegistry&     the_registry,
                      // other stuff:
                      std::ostream&    output)
/*
We tidy up the registry so that:
 - notes for unregistered interconnection networks have been dispensed with
 - notes for the same interconnection network have been amalgamated
 - interconnection networks with no note have had an empty note associated
 - there is at least one valid interconnection network (if not then fatal)
 - there is at least one valid traffic pattern (if not then fatal).

Errors:
 - Good, NoIntNetsRegistered, NoTPsRegistered, UnregNoteDeleted, AmalgamatedNotes, NoNoteForIntNet

Functions:
 - short_warning            from error_writing
 - fatal                    from error_writing
*/
{
    output << "\ntidying up the registry ...\n";

    // check for potentially fatal errors

    if (the_registry.int_nets_registered.empty())
    {
        output << " - " << short_warning(Errors::NoIntNetsRegistered) << "\n";
        fatal(Errors::FatalRegistry);
    }
    if (the_registry.traffic_patterns_registered.empty())
    {
        output << " - " << short_warning(Errors::NoTPsRegistered) << "\n";
        fatal(Errors::FatalRegistry);
    }

    // remove notes for unregistered interconnection networks

    for (auto it = the_registry.notes_registered.begin(); it != the_registry.notes_registered.end(); )
    {
        bool found = false;
        for (const auto& int_net : the_registry.int_nets_registered)
        {
            if (it->name == int_net.name)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            output << " - " << short_warning(Errors::UnregNoteDeleted, it->name) << "\n";
            it = the_registry.notes_registered.erase(it);
        }
        else
            ++it;
    }

    // amalgamate notes for the same interconnection network

    for (auto it1 = the_registry.notes_registered.begin(); it1 != the_registry.notes_registered.end(); ++it1)
    {
        auto next_it = std::next(it1);
        for (auto it2 = next_it; it2 != the_registry.notes_registered.end(); )
        {
            if (it2->name == it1->name)
            {
                it1->note += " " + it2->note;
                it2 = the_registry.notes_registered.erase(it2);
                output << " - " << short_warning(Errors::AmalgamatedNotes, it1->name) << "\n";
            }
            else
                ++it2;
        }
    }

    // add empty note for missing interconnection network notes

    for (const auto& int_net : the_registry.int_nets_registered)
    {
        bool found = false;
        for (const auto& note : the_registry.notes_registered)
        {
            if (int_net.name == note.name)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            RegNote new_note;
            new_note.name = int_net.name;
            new_note.note = "";
            the_registry.notes_registered.push_back(new_note);
            output << " - " << short_warning(Errors::NoNoteForIntNet, new_note.name) << "\n";
        }
    }

    output << "... registry tidied up\n";
}

Errors expand_named_list(// supplied data:
                         vector<string>&   reg_dim_strings)
/*
'reg_dim_strings' will be the 'parameters' component from the entry from 'the_registry.int_nets_registered'
corresponding to 'IN.name', e.g., [x, y^x, z, p^z]. Using 'IN.param_names' and 'IN.param_values', we expand
'reg_dim_strings' continually checking that this expanded version and 'IN.param_names' are identical. It is
assumed that:
 - 'reg_dim_strings' is valid, i.e., all names are non-empty (including both parts of a composite name)
 - a non-composite name has not appeared so far
 - the prefix of a composite name has not appeared so far
 - the suffix of a composite name has appeared so far
(this is promised by check_dimension_names()). The error states whether 'IN.param_names' is valid or not.

Errors:
 - Good, BadDimName, BadExpandedDimName
*/
{
    size_t index = 0;                       // current index in 'IN.param_names' and 'IN.param_values'
    size_t pos;
    size_t number_of_names;
    string prefix;
    string suffix;

    for (size_t i = 0; i < reg_dim_strings.size(); ++i)// check each reg-dim-string in turn
    {
        pos = reg_dim_strings[i].find("*"); // look for '*'
        if (pos == std::string::npos)       // non-composite
        {
            if (IN.param_names[index] != reg_dim_strings[i])// need a direct match
                return Errors::BadDimName;
            else
                index++;                    //increment index
        }
        else                                // composite
        {                                   // note: all composite names are well-formed
            prefix = reg_dim_strings[i].substr(0, pos);// prefix name: y from y^x
            suffix = reg_dim_strings[i].substr(pos + 1);// suffix name: x from y^x
            for (size_t j = 0; j < index; ++j)// look for existing 'suffix' in 'IN.param_names'
            {
                if (IN.param_names[j] == suffix)// found 'suffix'
                {
                    number_of_names = IN.param_values[j];// get corresponding value of 'suffix'
                    break;
                }
            }
            for (size_t j = 0; j < number_of_names; ++j)// check the requisite number of param names
            {
                if (IN.param_names[index] != (prefix + std::to_string(j + 1)))// names of form y1, y2, ...
                    return Errors::BadExpandedDimName;
                else
                    index++;                //increment index
            }
        }
    }
    return Errors::Good;
}

void validate_core_int_net_data(// supplied data:
                                TheRegistry&     the_registry,
                                // other stuff:
                                std::ostream&    output)
/*
After tidying up the registry via tidy_up_registry(), we validate:
 - the interconnection network name, the routing algorithm name, the list of dimension names, the tp names
with the corresponding registry entry. Once validated:
 - 'RG.net_machine', 'RG.net_type', 'RG.net_build' 'RG.registry_note' are all set.
All that remains to do is to validate the traffic pattern data and the fault data that has been entered
which will be done after topology_numerics() has executed.

Functions:
 - tidy_up_registry             from here
 - fatal                        from error_writing
 - expand_named_list            from here
*/
{
    Errors error;
    size_t index;

    tidy_up_registry(the_registry, output);// terminates if no int net or tp

    auto it = std::find_if(the_registry.int_nets_registered.begin(), the_registry.int_nets_registered.end(),
                           [](const RegIntNet& t) {return t.name == IN.name;});
    if (it == the_registry.int_nets_registered.end())
        fatal(Errors::UnregIntNetName, IN.name);
    else
        index = std::distance(the_registry.int_nets_registered.begin(), it);

    auto it1 = std::find_if(the_registry.int_nets_registered[index].routing_algs.begin(),
                            the_registry.int_nets_registered[index].routing_algs.end(),
                            [](const string& t) {return t == RA.name;});
    if (it1 == the_registry.int_nets_registered[index].routing_algs.end())
        fatal(Errors::UnregRAName, RA.name);

    RG.net_machine = the_registry.int_nets_registered[index].net_machine;
    RG.net_type =    the_registry.int_nets_registered[index].net_type;
    RG.net_build =   the_registry.int_nets_registered[index].net_build;

    auto it2 = std::find_if(the_registry.notes_registered.begin(), the_registry.notes_registered.end(),
                            [](const RegNote& t) {return t.name == IN.name;});
    if (it2 == the_registry.notes_registered.end())
        fatal(Errors::IntNetWithNoNote, IN.name);
    else
        RG.registry_note = it2->note;

    error = expand_named_list(the_registry.int_nets_registered[index].parameters);
    if (error != Errors::Good)
        fatal(Errors::MismatchedParams);

    for (const auto& tp : TP.interim)
    {
        auto it = std::find_if(the_registry.traffic_patterns_registered.begin(),
                               the_registry.traffic_patterns_registered.end(),
                               [&](const string& t) {return t == tp.name;});
        if (it == the_registry.traffic_patterns_registered.end())
            fatal(Errors::UnregTpName, tp.name);
    }
}

/**
*************************
Validate traffic patterns
*************************
**/

Errors get_a_source_or_dest_per_type(// supplied data:
                                     TrafficPatternSourceDest&    a_source_dest,
                                     const bool                   is_a_source,
                                     const size_t                 node_type,
                                     // computed data:
                                     size_t&                      abs_id)
/*
Takes a struct TrafficPatternSourceDest 'a_source_dest' where:
 - the node is of terminal type 'node_type' (corresponds to "pr" or "se" always)
 - is a source or a destination via 'is_a_source'
and returns 'abs_id' as the corresponding absid. We assume that the node being described is as per
validate_traffic_patterns() below.

Note that eventually a source-destination range [s, d] in an expanded tp denotes that the nodes to be
considered are s, s+1, ..., d-1 but this is handled in validate_traffic_patterns() and not here.

Errors:
 - Good, OutOfRangeSize_t, BadLengthTuple, OutOfRangeTuple

Functions:
 - alg_to_numeric_ptr           points to a function from maps
 - coding                       from error_writing
*/
{
    Errors error;

    Maps::AlgToNumericFn alg_to_numeric_ptr;        // set up correct association of algids to relids
    alg_to_numeric_ptr = M.alg_to_numeric[IN.name]; // 'alg_to_numeric_ptr' is the correct function

    if (std::holds_alternative<size_t>(a_source_dest.id_or_tuple))
    {
        abs_id = std::get<size_t>(a_source_dest.id_or_tuple);
        abs_id += G.node_types[node_type].first;
        if (abs_id < G.node_types[node_type].first)
        {
            abs_id = G.node_types[node_type].first;
            error = Errors::OutOfRangeSize_t;
        }
        else if (abs_id >= G.node_types[node_type].last)// note that '.last' = last node + 1
        {
            abs_id = G.node_types[node_type].last - 1;
            error = Errors::OutOfRangeSize_t;
        }
        else
            error = Errors::Good;
    }
    else if (std::holds_alternative<vector<size_t>>(a_source_dest.id_or_tuple))
    {
        vector<size_t> the_tuple = std::get<vector<size_t>>(a_source_dest.id_or_tuple);
        error = alg_to_numeric_ptr(node_type, the_tuple, abs_id);
        if (error != Errors::Good)
        {
            if (is_a_source)
                abs_id = G.node_types[node_type].first;
            else
                abs_id = G.node_types[node_type].last - 1;// note that '.last' = last node + 1
            a_source_dest.prefix = "default";
        }
        else
            error = Errors::Good;
    }
    else
    {
        error = Errors::TypeMismatch;
        coding(error);
    }
    return error;
}

Errors get_a_source_or_dest(// supplied data:
                            TrafficPatternSourceDest&    a_source_dest,
                            const bool                   is_a_source,
                            const bool                   is_a_start,
                            // computed data:
                            size_t&                      abs_id)
/*
Takes a struct TrafficPatternSourceDest 'a_source_dest' where:
 - the node is of terminal type "pr", "se", "*" or "default"
 - is a source or a destination via 'is_a_source'
 - a start or an end node via 'is_a_start'
and returns the corresponding absid. We assume that the node being described is as per
validate_traffic_patterns() below.

Note that if 'a_source_dest.prefix' = "*" and 'a_source_dest.id_or_tuple' is a vector<size_t> then after
we have determined what "*" should be, we set 'a_source_dest.prefix' to "pr" or "se" accordingly.

Note that eventually a source-destination range [s, d] in an expanded tp denotes that the nodes to be
considered are s, s+1, ..., d-1 but this is handled in validate_traffic_patterns() and not here.

Functions:
 - get_a_source_or_dest_per_type            from here
*/
{
    Errors error = Errors::Good;

    // "default" entry

    if (a_source_dest.prefix == "default")
    {
        if (is_a_start)
            abs_id = 0;
        else
            abs_id = G.num_terms - 1;
        return error;
    }

    // "*" entry

    if (a_source_dest.prefix == "*")
    {
        if (std::holds_alternative<vector<size_t>>(a_source_dest.id_or_tuple))
        {
            for (size_t nt = 0; nt < G.num_term_types; ++nt)
            {
                if (G.node_types[nt].number > 0)
                {
                    a_source_dest.prefix = G.node_types[nt].short_name;
                    break;
                }
            }
        }
        else
        {
            if (std::holds_alternative<size_t>(a_source_dest.id_or_tuple))
                abs_id = std::get<size_t>(a_source_dest.id_or_tuple);
            else
                coding(Errors::TypeMismatch);
            if (abs_id >= G.num_terms)
            {
                abs_id = G.num_terms - 1;
                error = Errors::OutOfRangeSize_t;
            }
            return error;
        }
    }

    // "pr" or "se" entry

    for (size_t nt = 0; nt < G.num_term_types; ++nt)
    {
        if (a_source_dest.prefix == G.node_types[nt].short_name)
        {
            error = get_a_source_or_dest_per_type(a_source_dest, is_a_source, nt, abs_id);
            return error;
        }
    }

    return error;       // this line is never reached
}

void validate_traffic_patterns(// supplied/computed data:
                               vector<Tag>&    config_tags)
/*
Undertake final validation of the interim tps in 'TP.interim'. For each struct TrafficPatternInterim 'tp' in
'TP.interim', we need to translate all source/destination entries into valid absids. We have already checked
that the traffic pattern names have been registered.

A struct TrafficPatternInterim 'tp' in the vector 'TP.interim' describes a tp after it has been read from the
config file but before it has been fully validated. Note that any valid such struct will have a non-empty
name and both 'tp.num_iterations' and 'tp.num_repeats' integers > 0. Also, the 4 items in 'tp_sources_dests'
will be descriptions of the entries for start-source, end-source, start-dest and end-dest via:
 - 'tp.prefix' = "pr", "se", "*" or "default"
 - 'tp.id_or_tuple' =
    - if 'tp.prefix' = "default" then this value will be empty
    - if 'tp.prefix' != "default":
      - if the value is a vector v then the node has relid v where:
        - if 'tp.prefix' = "pr" or "se" then v describes a processor or a server accordingly
        - if 'tp.prefix' = "*" then
          - v describes node of first node type "xx" where 'G.num_nodes'["xx"] > 0
      - if the value is a size_t v:
        - if prefix = "pr" or "se" then v describes a relid of corresponding node type
        - if prefix = "*" then v describes an absid.
The data is additionally validated in this function now that the registry has been read and
topology_numerics() has been executed.

Note that when supplied, the range of, e.g., sources is [start-source...end-source]. However, when we
repackage them below at (*), we will add 1 to end-source and interpret the source range as start-source <=
source < end-source (ditto for destinations).

The number of iterations is the number of times the traffic pattern is to be run, e.g., a random tp might be
run a certain number of times whereas an all-to-all tp would ordinarily just be run once. Given the pair
<tp name, #-iterations>, the number of repeats #-repeats states how many times this pair should be run, e.g.,
<random, <100, 5, ?>> would repeat <random, 100> 5 times. We might choose #-repeats = 5 so as to run the same
traffic pattern 5 times but each time with a different set of faults. On expansion, we obtain #-repeats of
<tp name, #-iterations>; that is, we treat this as #-repeats separate traffic patterns. For one thing, this
allows us to signal to the user when these tps have completed and we can also obtain comparative data for
each of the #-repeat traffic patterns.

For any one of these #-repeats traffic patterns: if #-iterations = 1 then we split the number of sources
(only) in the traffic pattern into percentage-batch blocks and regard these blocks as separate traffic
patterns (to all of the destinations); if #-iterations > 1 then we split the #-iterations traffic pattern
into percentage-block batches. so, in general this gives a number of struct TrafficPatternExpanded. E.g.,
 - <all-to-all, <1, 8, 25%>, <s1, t1, s2, t2>> results in first 8 copies of <all-to-all, 1> and for each copy,
   4 traffic patterns of the form <all-to-all,<1, s1, u1, s2, t2+1>>, <all-to-all, <1, u1+1, u2, s2, t2+1>>,
   <all-to-all, <1, u2+1, u3, s2, t2+1>>, <all-to-all, <1, u3+1, t1+1, s2, t2+1>>, i.e., 32 tps in total.
 - <random, <100, 20, 25%>, <s1, t1, s2, t2>> results in first 20 copies of <random, 100> with each copy
   split into 4 traffic patterns of the form <random, <25, s1, t1+1, s2, t2+1>>, i.e., 80 tps in total.
The resulting struct 'TP.expanded' is taken as the list of all traffic patterns constituting the run. So,
whatever the case, for any initial traffic pattern 'tp' in 'TP.interim', there are:
 - (#-repeats x ceil(100 / tp.percentage)) tps in 'TP.expanded'.
Different traffic patterns dictate different splits, e.g., for permutation traffic it does not usually make
sense to have a percentage batch of < 100 and #-iterations is usually 1 unless we are splitting just so as to
signal completions to the user. Moreover, generating weird traffic patterns might be computationally
challenging.

Traffic patterns can be batched together. E.g., if we have 20 tps then maybe we have two batches of 10: the
first batch consisting of the same tp on the same fault file; and the second batch consisting of the same tp
on a different fault file. The average, e.g., average hop length, is calculated across each batch. So, the
first batch might be where all fault files are 10% channel faults and the second batch where all fault files
are 10% node faults. On input 'TP.batches' should be of the form [x0, x1, ..., xd] so that x0 + x1 + ... + xd
= total number of traffic patterns = 'TP.expanded.size()' and which is changed into [0, y1, y2, ..., y(d+1)],
i.e., 0 -> y1 - 1, y1 -> y2 - 1, ... so that y1 = x0 - 1; y2 = x1 + x0 - 1 = x1 + y1; y3 = x2 + x1 + x0 - 1 =
x2 + y2, etc. If there is a mismatch between 'TP.batches' and 'TP.expanded.size()' then we set 'TP.batches'
as [0, 'TP.expanded.size()'], i.e., 1 batch only.

Data is always collected and written to output per batch of tps. If 'ENV.suppress_tps' = false then
latency/load/activity data is also written for each individual tp, otherwise it is not.

Functions:
 - warning                      from error_writing
 - get_a_source_or_dest         from here
*/
{
    Errors error;
    size_t abs_id;
    string name;
    size_t num_iterations;
    size_t start_source;
    size_t end_source;
    size_t start_dest;
    size_t end_dest;

    for (auto& tp : TP.interim)       // for each interim tp, build temp list of expanded tps
    {                                 //  before splitting by percentage batches
        // set num_iterations

        name = tp.name;
        num_iterations = tp.num_iterations;

        // get start-source, end-source, start-dest and end-dest

        error = get_a_source_or_dest(tp.sources_dests[0], true, true, abs_id);// get start-source
        start_source = abs_id;
        if (error != Errors::Good)
            tp.sources_dests[0].prefix = "default";
        error = get_a_source_or_dest(tp.sources_dests[1], true, false, abs_id);// get end-source
        end_source = abs_id;
        if (error != Errors::Good)
            tp.sources_dests[1].prefix = "default";
        error = get_a_source_or_dest(tp.sources_dests[2], false, true, abs_id);// get start-dest
        start_dest = abs_id;
        if (error != Errors::Good)
            tp.sources_dests[2].prefix = "default";
        error = get_a_source_or_dest(tp.sources_dests[3], false, false, abs_id);// get end-dest
        end_dest = abs_id;
        if (error != Errors::Good)
            tp.sources_dests[3].prefix = "default";

        // check that source-dest ranges are legitimate

        if (end_source < start_source)
        {
            warning(Errors::IllFormedtp, tp.name);
            start_source = 0;
            end_source = G.num_terms - 1;
        }
        if (end_dest < start_dest)
        {
            warning(Errors::IllFormedtp, tp.name);
            start_dest = 0;
            end_dest = G.num_terms - 1;
        }

        // build percentage

        size_t source_fraction = ((end_source - start_source + 1) * tp.percentage + 99) / 100;
        size_t iteration_fraction = (num_iterations * tp.percentage + 99) / 100;

        TrafficPatternExpanded expanded_tp;             // an expanded tp that is built here
        vector<TrafficPatternExpanded> temp_list_of_tps;// a temporary list of expanded tps
        size_t fixed_end_source;
        size_t start_batch;

        // expand tps w.r.t. percentage

        expanded_tp.name = name;                        // form skeleton for all expanded tps
        expanded_tp.start_dest = start_dest;
        expanded_tp.end_dest = end_dest;
        if (num_iterations == 1)                        // partition sources
        {
            temp_list_of_tps.reserve((end_source - start_source + 1) / source_fraction + 1);
            fixed_end_source = end_source;              // remember end-source
            expanded_tp.num_iterations = num_iterations;
            while (start_source <= fixed_end_source)    // loop through batches
            {
                if (start_source + source_fraction - 1 <= fixed_end_source)
                    end_source = start_source + source_fraction - 1;// 'source_fraction' sources in the batch
                else
                    end_source = fixed_end_source;      // < 'source_fraction' sources in the batch
                expanded_tp.start_source = start_source;// set start-source and end-source in 'extended_tp'
                expanded_tp.end_source = end_source;
                temp_list_of_tps.push_back(expanded_tp);// push 'extended_tp' onto temporary list
                start_source = start_source + source_fraction;// move to next batch (if there is one)
            }
        }
        else                                            // partition iterations
        {
            temp_list_of_tps.reserve(num_iterations / iteration_fraction + 1);
            start_batch = 0;
            expanded_tp.start_source = start_source;
            expanded_tp.end_source = end_source;
            while (start_batch <= tp.num_iterations - 1)// loop through batches
            {
                if (start_batch + iteration_fraction - 1 <= tp.num_iterations - 1)
                    expanded_tp.num_iterations = iteration_fraction;// 'iteration_fraction' in batch
                else
                    expanded_tp.num_iterations = tp.num_iterations - start_batch;// < 'iteration_fraction'
                temp_list_of_tps.push_back(expanded_tp);// push 'extended_tp' onto temporary list
                start_batch = start_batch + iteration_fraction;
            }
        }

        // add 1 to end nodes (*)

        for (auto& temp_tp : temp_list_of_tps)
        {
            temp_tp.end_source++;
            temp_tp.end_dest++;
        }

        // add in #-repeats

        for (size_t i = 0; i < tp.num_repeats; ++i)     // remember #-repeats copies of tps
        {
            for (auto& temp_tp : temp_list_of_tps)
            {
                TP.expanded.push_back(temp_tp);
            }
        }
    }

    // sort out 'TP.batches'

    size_t sum_of_batch_numbers = 0;
    for (const auto& batch_number : TP.batches)
    {
        sum_of_batch_numbers = sum_of_batch_numbers + batch_number;
    }
    if (sum_of_batch_numbers != TP.expanded.size())
        TP.batches = {0, TP.expanded.size()};
    else
    {
        vector<size_t> new_batches;
        new_batches.reserve(TP.batches.size() + 1);
        new_batches.push_back(0);
        for (size_t i = 1; i < TP.batches.size() + 1; ++i)
        {
            if (i == 1)
                new_batches.push_back(TP.batches[0]);
            else
                new_batches.push_back(new_batches[i - 1] + TP.batches[i - 1]);
        }
        TP.batches = new_batches;
        for (auto& tag : config_tags)
        {
            if (tag.name == "<TRAFFICPATTERN>")
            {
                for (auto& subtag : tag.subtags)
                {
                    if (subtag.name == "batches")
                    {
                        subtag.value_read = true;
                        break;
                    }
                }
                break;
            }
        }
    }
}

/**
***************************
Validate fault descriptions
***************************
**/

size_t get_number_of_node_type(// supplied data:
                               string&    short_name)
/*
Return the number of nodes of type 'short_name' which is in {"pr", "se", "sw", "os", "*"}. Note that the
valid number will always be returned.
*/
{
    for (size_t nt = 0; nt < G.num_node_types; ++nt)
    {
        if (G.node_types[nt].short_name == short_name)
            return G.node_types[nt].number;
    }
    return G.total_nodes;
}

size_t get_number_of_channel_type(// supplied data:
                                  string&    source_short_name,
                                  string&    dest_short_name)
/*
Return the number of channels of type 'source_short_name''dest_short_name' where both 'source_short_name'
and 'dest_short_name' are in {"pr", "se", "sw", "os", "*"}. Note that the valid number will always be
returned.
*/
{
    if ((source_short_name != "*") && (dest_short_name != "*"))
    {
        string channel_name = source_short_name + dest_short_name;
        for (size_t ct = 0; ct < G.num_channel_types; ++ct)
        {
            if (G.channel_types[ct].short_name == channel_name)
                return G.channel_types[ct].number;
        }
    }
    else if ((source_short_name == "*") && (dest_short_name != "*"))
    {
        size_t num_channels = 0;
        string channel_name;
        for (size_t nt = 0; nt < G.num_node_types; ++nt)
        {
            channel_name = G.node_types[nt].short_name + dest_short_name;
            for (size_t ct = 0; ct < G.num_channel_types; ++ct)
            {
                if (G.channel_types[ct].short_name == channel_name)
                {
                    num_channels += G.channel_types[ct].number;
                    break;
                }
            }
        }
        return num_channels;
    }
    else if ((source_short_name != "*") && (dest_short_name == "*"))
    {
        size_t num_channels = 0;
        string channel_name;
        for (size_t nt = 0; nt < G.num_node_types; ++nt)
        {
            channel_name = source_short_name + G.node_types[nt].short_name;
            for (size_t ct = 0; ct < G.num_channel_types; ++ct)
            {
                if (G.channel_types[ct].short_name == channel_name)
                {
                    num_channels += G.channel_types[ct].number;
                    break;
                }
            }
        }
        return num_channels;
    }
    return G.total_channels;
}

void validate_fault_descriptions()
/*
The vector of FaultDescription is held in 'FAU.fault_description'. Here, a check of any 'raw_count' is made
to make sure that the number of relevant nodes/channels/links is greater than the number of faults to be
set. If a bad fault description is found then it is removed from 'FAU.fault_description'. Alternatively,
the number of faults to be set, be it 'raw_count' or 'percentage', is registered in 'number_of_faults'.
Note that it only makes sense to set links as faulty when the interconnection network is a full duplex
undirected graph.

Functions:
 - warning                              from error_writing
 - get_number_of_node_type              from here
 - get_number_of_channel_type           from here
*/
{
    for (auto it = FAU.fault_description.begin(); it != FAU.fault_description.end(); )
    {
        if (it->name == "node")
        {
            size_t num_nodes = get_number_of_node_type(it->source_node_type);
            if (it->raw_count >= num_nodes)
            {
                warning(Errors::AllFaults, "nodes");
                it = FAU.fault_description.erase(it);
                continue;
            }
            else
            {
                if (it->raw_count == 0)
                    it->number_of_faults = static_cast<size_t>(it->percentage * num_nodes / 100);
                else
                    it->number_of_faults = it->raw_count;
                ++it;
            }
        }
        else if (it->name == "channel")
        {
            size_t num_channels = get_number_of_channel_type(it->source_node_type, it->dest_node_type);
            if (it->raw_count >= num_channels)
            {
                warning(Errors::AllFaults, "channels");
                it = FAU.fault_description.erase(it);
                continue;
            }
            else
            {
                if (it->raw_count == 0)
                    it->number_of_faults = static_cast<size_t>(it->percentage * num_channels / 100);
                else
                    it->number_of_faults = it->raw_count;
                ++it;
            }
        }
        else if (it->name == "link")
        {
            size_t num_links = get_number_of_channel_type(it->source_node_type, it->dest_node_type) / 2;
            if (it->raw_count >= num_links)
            {
                warning(Errors::AllFaults, "links");
                it = FAU.fault_description.erase(it);
                continue;
            }
            else
            {
                if (it->raw_count == 0)
                    it->number_of_faults = static_cast<size_t>(it->percentage * num_links / 100);
                else
                    it->number_of_faults = it->raw_count;
                ++it;
            }
        }
    }
}

/**
****************
Rewrite registry
****************
**/

void rewrite_the_registry(// supplied data:
                          const string&              input_file_name,
                          vector<ValueAndTriple>&    values_and_triples,
                          string&                    pre_registry,
                          string&                    post_registry,
                          TheRegistry&               the_registry)
/*
Replace the registry "registry.txt" (say) with a tidied-up version of the same name and save the old
version as "registry_old.txt". Note that I assume that "input_file_name" is valid and ends in ".txt".
*/
{
    string stem_file_name = input_file_name.substr(0, input_file_name.size() - 4);
    string temp_file_name = stem_file_name + "_tmp.txt";
    string old_file_name = stem_file_name + "_old.txt";

    std::ofstream tmp(temp_file_name);
    if (!tmp)
        fatal(Errors::RegistryRewriteFail);

    string to_be_written = pre_registry;
    to_be_written += "<INTERCONNECTIONNETWORK/ROUTINGALGORITHM>\n";
    for (const auto& int_net : the_registry.int_nets_registered)
    {
        to_be_written += int_net.name + "[";
        for (auto it = int_net.parameters.begin(); it != int_net.parameters.end(); ++it)
        {
            to_be_written += *it;
            if (std::next(it) != int_net.parameters.end())
                to_be_written += "-";
        }
        to_be_written += ", ";

        auto it1 = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                                [&](const ValueAndTriple& item)
                                {
                                    return item.value == GlobalParamVal{int_net.net_machine};
                                });
        if (it1 != values_and_triples.end())
            to_be_written += it1->triple.value + ", ";
        else
            coding(Errors::TypeMismatch);

        auto it2 = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                                [&](const ValueAndTriple& item)
                                {
                                    return item.value == GlobalParamVal{int_net.net_type};
                                });
        if (it2 != values_and_triples.end())
            to_be_written += it2->triple.value + ", ";
        else
            coding(Errors::TypeMismatch);

        auto it3 = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                                [&](const ValueAndTriple& item)
                                {
                                    return item.value == GlobalParamVal{int_net.net_build};
                                });
        if (it3 != values_and_triples.end())
            to_be_written += it3->triple.value + "]<";
        else
            coding(Errors::TypeMismatch);

        for (auto it = int_net.routing_algs.begin(); it != int_net.routing_algs.end(); ++it)
        {
            to_be_written += *it;
            if (std::next(it) != int_net.routing_algs.end())
                to_be_written += ", ";
        }
        to_be_written += ">\n";
    }

    to_be_written += "\n<TRAFFICPATTERN>\n";
    for (auto it = the_registry.traffic_patterns_registered.begin(); it != the_registry.traffic_patterns_registered.end(); ++it)
    {
        to_be_written += *it;
        if (std::next(it) != the_registry.traffic_patterns_registered.end())
            to_be_written += ", ";
    }
    to_be_written += "\n";

    to_be_written += "\n<NOTES>\n";
    for (const auto& a_note : the_registry.notes_registered)
    {
        to_be_written += a_note.name + " = " + a_note.note + "\n";
    }

    to_be_written += "\n<ENDOFFILE>\n";

    to_be_written += post_registry;

    tmp << to_be_written;
    tmp.close();

    if (std::filesystem::exists(input_file_name))
        std::filesystem::rename(input_file_name, old_file_name);

    std::filesystem::rename(temp_file_name, input_file_name);
}









