
#include <algorithm>

#include "traffic_generators.h"
#include "../random_gen.h"
#include "../globals.h"

using std::vector;
using std::string;
using std::size_t;
using std::pair;

/**
This cpp contains functions concerning traffic generators. Every traffic generator is of the form:
 - <tp-name>_traffic_generator()
and works in one of two ways:
 - it takes the current source-destination pair and calculates and returns the next source-destination
   pair
 - it works with a precomputed set of source-destination pairs and returns the next one after the current
   index.
Every traffic generator needs to be supplied with:
 - tp_number            - the current tp number so that we have access to the traffic pattern
 - started              - set as true if this is not the first source-dest pair
 - source               - the current source (returned as the next one)
 - dest                 - the current dest (returned as the next one)
 - precomputed_tp       - access to a precomputed bespoke traffic pattern
 - index                - the current pair in a bespoke traffic pattern
 - output               - the screen
False is returned when we have finished all source-dest pairs. Note that we may assume that any start-end
source or dest range is non-empty.

Functions:
 - random_traffic_generator()
 - alltoall_traffic_generator()
 - generate_a_permutation_tp()
 - permutation_traffic_generator()
**/

bool random_traffic_generator(// supplied data:
                              const size_t                           tp_number,
                              //supplied/computed data:
                              bool&                                  started,
                              size_t&                                source,
                              size_t&                                dest,
             [[maybe_unused]] vector<std::pair<size_t, size_t>> &    precomputed_tp,
             [[maybe_unused]] size_t&                                index)
/*
The random traffic generator. Given ['source','dest']:
 - the next pair [++'source'/guessed 'dest'] is computed, unless this is the first call when
   ['source','dest'] = [start_source, guessed dest] is computed.

Functions:
 - RNG_tp.getSizeT          from random_gen
*/
{
    if (!started)
    {
        source = TP.expanded[tp_number].start_source;
        started = true;
    }
    else
        ++source;
    if (source == TP.expanded[tp_number].end_source)
        return false;
    dest = RNG_tp.getSizeT(TP.expanded[tp_number].start_dest,
                           TP.expanded[tp_number].end_dest - 1);
    return true;
}

bool alltoall_traffic_generator(// supplied data:
                                const size_t                          tp_number,
                                //supplied/computed data:
                                bool&                                 started,
                                size_t&                               source,
                                size_t&                               dest,
               [[maybe_unused]] vector<std::pair<size_t, size_t>>&    precomputed_tp,
               [[maybe_unused]] size_t&                               index)
/*
The alltoall traffic generator. Given ['source','dest'];
 - the lexicographically-next pair ['source'/'dest']++ is computed, unless this is the first call when
   ['source','dest'] = start_source/start_dest is computed.
*/
{
    if (!started)
    {
        source = TP.expanded[tp_number].start_source;
        dest = TP.expanded[tp_number].start_dest;
        started = true;
    }
    else
    {
        ++dest;
        if (dest == TP.expanded[tp_number].end_dest)
        {
            dest = TP.expanded[tp_number].start_dest;
            ++source;
        }
    }
    if (source == TP.expanded[tp_number].end_source)
        return false;
    return true;
}

void generate_a_permutation_tp(// supplied data:
                               const size_t                          tp_number,
                               // supplied/computed data:
                               vector<std::pair<size_t, size_t>>&    precomputed_tp)
/*
Build a permutation traffic pattern for the expanded tp of number 'tp_number' in 'TP.expanded'. If the
start-range is bigger than the dest-range then a fatal error is flagged.

Functions:
 - RNG_tp.get           from random_gen
*/
{
    const auto& tp = TP.expanded[tp_number];

    size_t start_source = tp.start_source;
    size_t end_source   = tp.end_source;
    size_t start_dest   = tp.start_dest;
    size_t end_dest     = tp.end_dest;
    size_t size_start_range = end_source - start_source;
    size_t size_dest_range  = end_dest   - start_dest;

    if (size_start_range > size_dest_range)
        fatal(Errors::InvalidtpDesc, "tp number " + std::to_string(tp_number));

    precomputed_tp.clear();
    precomputed_tp.reserve(size_start_range);
    std::vector<size_t> dests;
    dests.reserve(size_dest_range);
    for (size_t j = start_dest; j < end_dest; ++j)
    {
        dests.push_back(j);
    }
    std::shuffle(dests.begin(), dests.end(), RNG_tp.get());
    for (size_t i = 0; i < size_start_range; ++i)
    {
        size_t source = start_source + i;
        size_t dest   = dests[i];
        precomputed_tp.emplace_back(source, dest);
    }
}

bool permutation_traffic_generator(// supplied data:
                                   const size_t                          tp_number,
                                   //supplied/computed data:
                                   bool&                                 started,
                                   size_t&                               source,
                                   size_t&                               dest,
                                   vector<std::pair<size_t, size_t>>&    precomputed_tp,
                                   size_t&                               index)
/*
The permutation traffic generator. This function uses a precomputed permutation traffic pattern in
'precomputed_tp' and simply returns the next pair, i.e., the 'index'++ pair, unless the call is the
first call when the actual traffic pattern is precomputed and pair-0 is returned. False is returned
if all source-dest pairs from 'precomputed_tp' have been handled, otherwise true.

Functions:
 - generate_a_permutation_tp            from here
*/
{
    if (!started)
    {
        generate_a_permutation_tp(tp_number, precomputed_tp);
        index = 0;
        source = precomputed_tp[index].first;
        dest = precomputed_tp[index].second;
        started = true;
    }
    else
    {
        ++index;
        if (index == precomputed_tp.size())
            return false;
        else
        {
            source = precomputed_tp[index].first;
            dest = precomputed_tp[index].second;
        }
    }
    return true;
}
