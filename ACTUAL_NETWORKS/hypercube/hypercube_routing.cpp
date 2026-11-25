
#include <algorithm>

#include "hypercube_routing.h"
#include "hypercube.h"
#include "../../globals.h"
#include "../../random_gen.h"

using std::vector;
using std::size_t;

/**
In order to introduce a new routing algorithm, the following functions need to be supplied:
 - <int_net>_routing_prelude()
 - <int_net>_distance()
 - <int_net>_<routing_alg>().
Other functions can be supplied too but they will be local to this cpp. If there is some data that
is best computed once only and made available for use thereafter then this should be defined in
the anonymous namespace and computed in <int_net>_routing_prelude(). Note that there is only one
"prelude" function no matter how many routing algorithms there are.

Note that do_a_shuffle() is used when we want to compute a random dimension-order on the fly. It
should be left as is and should always appear.

Note that we include error_writing.h. A user can cover themselves for coding errors and should use
the general errors Errors::BuildingError and Errors::ExecutionError.
**/

namespace
/**
This anonymous namespace enables ready access to the dimension n > 0 which is always defined via the
instruction:
 - size_t n = IN.param_values[0];
in <int_net>_routing_prelude(). Other values can be included too, e.g.,
 - for k-ary n-cubes, n and k might appear in the anonymous namespace
with the additional instruction
 - k = IN.param_values[1];
in <int_net>_routing_prelude().
**/
{
    /** LEAVE INSTRUCTIONS AS ARE **/

    size_t n;

    [[maybe_unused]] void do_a_shuffle()
    /**
    Randomize the dimension order.
    **/
    {
        shuffle(RAP.dim_order.begin(), RAP.dim_order.end(), RNG_routing.get());
    }

    /** USER-DEFINED **/

    // discretionary aliases can be added below to make life easier for the user, e.g.,
    //     const auto &pr = G.pr;
    //     const auto &proc = G.node_types[pr];
}

void hypercube_routing_prelude()
/**
Include preliminary data here, e.g., dimension-orders (note that we always assume that there are n
dimensions).
**/
{
    /** LEAVE INSTRUCTIONS AS ARE **/

    n = IN.param_values[0];             // the dimension n is always 'IN.param_values[0]'

    RAP.dim_order.reserve(n);
    if (RA.dor == RoutingDor::asc)
    {
        for (size_t i = 0; i < n; ++i)
        {
            RAP.dim_order.push_back(i);
        }
    }
    else if (RA.dor == RoutingDor::des)
    {
        for (size_t i = 0; i < n; ++i)
        {
            RAP.dim_order.push_back(n - 1 - i);
        }
    }
    else if (RA.dor == RoutingDor::ran)
    {
        for (size_t i = 0; i < n; ++i)
        {
            RAP.dim_order.push_back(i);
        }
        shuffle(RAP.dim_order.begin(), RAP.dim_order.end(), RNG_routing.get());
    }

    /** USER-DEFINED **/
}

void hypercube_dor_routing(// supplied data:
                           vector<node>&           int_net,
                           const size_t            source,
                           const size_t            destination,
                           // computed data:
                           struct RouteStruct&    route)
/**
Basic dimension-order routing. We assume that 'source' and 'destination' are legal and that an educated
guess has been made so as to reserve some space for 'route.port_route'. It is pointless supplying
'RA.mode' as the algorithm as implemented is always deterministic.

Functions:
 - hypercube_alg_convert_from_id            from hypercube (via alg_from_numeric_ptr)
**/
{
    /** LEAVE INSTRUCTIONS AS ARE **/

    [[maybe_unused]] Errors error;

    Maps::AlgFromNumericFn alg_from_numeric_ptr;        // pointer to alg-from-numeric function
    auto alg_from_numeric_it = M.alg_from_numeric.find(IN.name);
    if (alg_from_numeric_it == M.alg_from_numeric.end())
        coding(Errors::BadRoutingAlg);
    else
        alg_from_numeric_ptr = alg_from_numeric_it->second;

    size_t source_type = int_net[source].node_get_type();// type of source
    size_t dest_type = int_net[destination].node_get_type();// type of destination

    route.source = source;      // set 'source' as the route source
    route.port_route.clear();   // clear port list (space has already been reserved)
    route.error_flag = {Errors::Good, 0};// assume all is fine

    vector<size_t> source_alg;  // convert 'source' and 'destination'
    source_alg.reserve(n);
    error = alg_from_numeric_ptr(source_type, source, source_alg);
    vector<size_t> destination_alg;
    destination_alg.reserve(n);
    error = alg_from_numeric_ptr(dest_type, destination, destination_alg);

    if (source == destination)  // so can assume source != dest throughout
    {
        if ((FAU.fault_check == FaultCheck::interim_en_route)
            || (FAU.fault_check == FaultCheck::interim_at_end)
            || (FAU.fault_check == FaultCheck::interim_en_route_and_post)
            || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
        {
            if (int_net[source].node_get_health() != G.healthy)
                route.error_flag = {Errors::FaultySource, 0};
        }
        return;
    }

    /** USER-DEFINED **/

    if (RA.dor == RoutingDor::ranperpair)               // build a random dimension order in 'RAP.dim_order'
        do_a_shuffle();

    route.port_route.clear();
    for (size_t i = 0; i < n; ++i)                      // for each dimension/port in turn ...
    {
        if (source_alg[RAP.dim_order[i]] != destination_alg[RAP.dim_order[i]])// if source-dest bits differ
            route.port_route.push_back(RAP.dim_order[i]);//  then add the port to the vector of ports
    }
    route.error_flag = {Errors::Good, 0};               // assume healthy
    return;
}
