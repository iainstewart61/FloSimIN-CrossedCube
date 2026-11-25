
#include <algorithm>

#include "crossedcube_routing.h"
#include "crossedcube.h"
#include "../../globals.h"
#include "../../random_gen.h"
#include "../../construction/int_net_construction.h"
#include "../../errors/error_writing.h"

using std::vector;
using std::size_t;
using std::pair;
using std::tuple;
using std::string;
using std::map;
using std::make_pair;
using std::make_tuple;

/**
core functions:
 - crossedcube_routing_prelude()
 - crossedcube_distance()

widely-applicable functions:
 - check_odd()
 - twiddle_double_bit()
 - flip_bit()
 - max_double_bit()

crossedcube_Efe:
 - max_non_twiddles()
 - build_orig_table()
 - state_debug()
 - crossedcube_Efe()

crossedcube_ChangSungHsu:
 - dp()
 - hamming_distance()
 - max_index_in_vector()
 - compute_rho()
 - comp_right()
 - comp_left()
 - OSR_full()
 - OSR_nondet()
 - OSR_det()
 - amend_OSR_bit_move_and_register()
 - precompute_j_phase_1()
 - precompute_i_j_phase_2()
 - get_j_for_OSR_phase_2()
 - register_dbits_from_Alg2_phase_2()
 - rewind_prior_to_non_det_dbit_OSR_move_phase_2()
 - state_phase_1()
 - state_phase_2()
 - phase_1()
 - phase_2()
 - initialization_phase_2()
 - initialization_phase_1()
 - crossedcube_ChangSungHsu()

crossedcube_Stewart:
 - build_table()
 - crossedcube_Stewart()

crossedcube_OtakeMouriKanedo_1:
 - get_distance()
 - build_Fwd_Swd_Bwd()
 - get_Fwd_Swd_Bkd()
 - method_1_full()
 - crossedcube_OtakeMouriKanedo_1()

crossedcube_OtakeMouriKanedo_2:
 -







**/

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

/**
Given the complexity of some of the routing algorithms, there is an option to run them in debug mode
whereby the execution walks through route construction so the user can see how the route is being
built and double-check the routing algorithm's validity. In order to do this, uncomment the following
instruction and recompile.
**/

//#define DEBUG_MODE

#ifdef DEBUG_MODE
#include <iostream>
#include "../../misc/misc_formatting_functions.h"
#include "../../output/writing_output.h"
#endif // DEBUG_MODE

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

void crossedcube_routing_prelude()
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

/**
*************************************************************************************
We first include functions that are relevant for all crossed cube routing algorithms.
*************************************************************************************
**/

bool check_odd(// supplied data:
               const size_t    a_size_t)
/*
Returns true if 'a_size_t' is odd, false if even.
*/
{
    if (a_size_t == (2 * (a_size_t / 2)))
        return false;
    else
        return true;
}

void twiddle_double_bit(// supplied data:
                        const size_t       dbit,
                        // supplied/computed data:
                        vector<size_t>&    u)
/*
Twiddle the 'dbit'-th d-bit of 'u' where 'twiddle' means flip 10 to 11 and 11 to 10 but leave 00 and
01 alone. We assume that 'dbit' <= number of d-bits - 1 and that d-bit 'dbit' consists of 2 bits (but
don't check this).
*/
{
    if (u[2 * dbit] == 1)           // assume that d-bit 'dbit' consists of 2 bits!
        u[2 * dbit + 1] = 1 - u[2 * dbit + 1];
}

void flip_bit(//supplied data:
              const size_t       the_bit,
              // supplied/computed data:
              vector<size_t>&    u)
/*
Flip the 'the_bit'-th bit of the node 'u' undertaking all the necessary downstream twiddles, i.e., build
the neighbour in dimension 'the_bit'. We assume that 'the_bit' <= n - 1 (but don't check this).

Functions:
 - twiddle_double_bit
*/
{
    u[the_bit] = 1 - u[the_bit];
    if (the_bit > 1)                            // if 'the_bit' = 0 or 1 we are done
    {
        size_t dbit = the_bit/2 - 1;            // get the downstream d-bits to twiddle
        for (size_t i = dbit + 1; i-- > 0; )    // work backwards to 0
        {
            twiddle_double_bit(i, u);           // twiddle d-bits
        }
    }
}

void max_double_bit(// supplied data:
                    const vector<size_t>&    source_alg,
                    const vector<size_t>&    destination_alg,
                    const size_t             num_dbits,
                    // computed data:
                    size_t&                  q,
                    size_t&                  m)
/*
Maximum d-bit 'q' where 'source' and 'destination' differ. Recall, 'num_dbits' = ceiling(n/2) so
it is always > 0. 'm' is the number of bits where the max d-bits differ (so 1 or 2) or m = 0 if
the 'source_alg' = 'destination_alg'. If 'm' = 0 then the value of 'q' is garbage (actually
'num_dbits' - 1).
*/
{
    m = 0;                              // initialize
    q = num_dbits - 1;                  // so dbits are 0, ..., 'q' = 'num_dbits'-1 >= 0
    if (!check_odd(n))                  // n is even so dbits are (0, 1), (2, 3), ..., (2q, 2q+1)
    {
        if ((source_alg[2 * q] != destination_alg[2 * q])
            && (source_alg[2 * q + 1] != destination_alg[2 * q + 1]))// last 2 bits different
            m = 2;
        else if ((source_alg[2 * q] != destination_alg[2 * q])
                 || (source_alg[2 * q + 1] != destination_alg[2 * q + 1]))// 1 of last 2 bits different
            m = 1;
    }
    else                                // n is odd so bits are (0, 1), (2, 3), ..., (2q-2,2q-1), 2q
    {
        if (source_alg[2 * q] != destination_alg[2 * q])// last bits different
            m = 1;
    }
    if ((m > 0) || (q == 0))            // right-most d-bits different or finished so return
        return;
    while (q > 0)                       // while we're not at the 0th d-bit ... (q > 0 at start)
    {
        --q;
        if ((source_alg[2 * q] != destination_alg[2 * q])// now 'q'th d-bit has 2 bits
            && (source_alg[2 * q + 1] != destination_alg[2 * q + 1]))// 2 bits different
            m = 2;
        else if ((source_alg[2 * q] != destination_alg[2 * q])
                 || (source_alg[2 * q + 1] != destination_alg[2 * q + 1]))// 1 bit different
            m = 1;
        if (m > 0)
            return;                     // d-bits different so return
    }
    m = 0;                              // finished - all d-bits are identical
}

/**
********************************************************************************************************
********************************************************************************************************
The 'crossedcube_Efe' routing algorithm from:
 - K. Efe, The crossed cube architecture for parallel computation.IEEE Transactions on Parallel and
   Distributed Systems, 3(5):513–524, September 1992.
The version of this routing algorithm that is cross-referenced is as laid out in I.A. Stewart, Minimal
routing in crossed cubes.
********************************************************************************************************
********************************************************************************************************
**/

size_t max_non_twiddles(// supplied data:
                        const vector<size_t>&    current_alg,
                        const vector<size_t>&    destination_alg,
                        const size_t             q)
/*
Finds the maximum d-bit 'p' to the left of d-bit 'q' so that d-bit 'p' of 'current_alg' is not
twiddle-related with d-bit 'p' of 'destination_alg' and returns this value. It is assumed that
0 <= 'q' < 'num_dbits' (which is not checked). If 'p' does not exist then 'p' is set at 'q'.
*/
{
    for (size_t p = q; p-- > 0; )
    {
        if ((current_alg[2 * p] == 0) && (destination_alg[2 * p] == 0)
            && (current_alg[2 * p + 1] == destination_alg[2 * p + 1]))// d-bits twiddle-related
            continue;
        else if ((current_alg[2 * p] == 1) && (destination_alg[2 * p] == 1)
                 && (current_alg[2 * p + 1] != destination_alg[2 * p + 1]))// d-bits twiddle-related
            continue;
        else            // not twiddle-related
            return p;
    }
    return q;
}

void build_orig_table(// computed data:
                      map<pair<pair<size_t, size_t>, pair<size_t, size_t> >, string>&    the_table)
/*
Build the move table.
*/
{
    the_table[{{0,0}, {0,0}}] = "";
    the_table[{{0,0}, {1,0}}] = "q";
    the_table[{{0,0}, {0,1}}] = "rq";
    the_table[{{0,0}, {1,1}}] = "l";
    the_table[{{1,0}, {0,0}}] = "l";
    the_table[{{1,0}, {1,0}}] = "rq";
    the_table[{{1,0}, {0,1}}] = "q";
    the_table[{{1,0}, {1,1}}] = "";
    the_table[{{0,1}, {0,0}}] = "rq";
    the_table[{{0,1}, {1,0}}] = "l";
    the_table[{{0,1}, {0,1}}] = "";
    the_table[{{0,1}, {1,1}}] = "q";
    the_table[{{1,1}, {0,0}}] = "q";
    the_table[{{1,1}, {1,0}}] = "";
    the_table[{{1,1}, {0,1}}] = "l";
    the_table[{{1,1}, {1,1}}] = "rq";
}

#ifdef DEBUG_MODE
void state_debug(// supplied data:
                 const vector<size_t>&        current_alg,
                 const vector<size_t>&        destination_alg,
                 const struct RouteStruct&    route,
                 const vector<int>&           non_det_flip)
/*
Print the debug state and wait for the user to hit return.

Functions:
 - vector_to_a_string           from misc_formatting_functions
*/
{
    std::cout << "the bits         = " << vector_to_a_string(route.port_route, 0) << "\n";
    std::cout << "the non-det bits = " << vector_to_a_string(non_det_flip, 0) << "\n";
    std::cout << "current_alg      = " << vector_to_a_string(current_alg, 0) << "\n";
    std::cout << "destination      = " << vector_to_a_string(destination_alg, 0) << "\n";
    std::cin.get();
}
#endif // DEBUG_MODE

void crossedcube_Efe(//supplied data:
                     vector<node>&          int_net,
                     const size_t           source,
                     const size_t           destination,
                     // computed data:
                     struct RouteStruct&    route)
/**
The original crossed cube routing algorithm from K. Efe, The crossed cube architecture for parallel
computation, IEEE Transactions on Parallel and Distributed Systems, 3(5):513–524, September 1992.
The algorithm is non-deterministic. When run so that:
 - 'RA.mode' = "nondet", it simply makes some guesses and returns the route (with 'RA.nondet_repeat' bad
                         runs allowed)
 - 'RA.mode' = "det",    it runs with the lowest guess at each stage
 - 'RA.mode' = "full",   it iterates through all possible guesses (in the case of faults).
Fault checking is undertaken within the routing algorithm if 'FAU.fault_check' = "interim_en_route" or
"interim_at_end" so that:
    - if "interim_en_route" then a route is built noting faulty components en route (so no fault-check of
      the final route is required)
    - if "interim_at_end" then a route is built ignoring faulty components (so a fault-check of the final
      route is required)
    - if "post" then regardless as to whether faults have been catered for in route building, a
      fault-check is undertaken externally when the route is returned
    - if "interim_en_route_and_post" then as for "interim_en_route" with an external "post" check too
      (used when debugging)
    - if "interim_at_end_and_post" then as for "interim_at_end" with an external "post" check too
      (used when debugging).
No "incremental" mode has been implemented. Only "full" mode has debugging information.

Functions called:
 - crossedcube_alg_convert_from_id          from crossedcube (via alg_from_numeric_ptr)
 - build_orig_table                         from here
 - max_double_bit                           from here
 - max_non_twiddles                         from here
 - flip_bit                                 from here
 - fault_check_route                        from node_port_related
 - vector_to_a_string                       from misc_formatting_functions
 - view_a_route                             from writing_output
 - fatal                                    from error_writing
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

    if (source == destination)  // so can assume source != dest
    {
        if ((FAU.fault_check == FaultCheck::interim_en_route)
            || (FAU.fault_check == FaultCheck::interim_at_end)
            || (FAU.fault_check == FaultCheck::interim_en_route_and_post)
            || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
        {
            if (int_net[source].node_get_health() != G.healthy)
                route.error_flag = {Errors::FaultySource, 0};
        }
        ++route.times_tried;
        return;
    }

    /** USER-DEFINED **/

    if (RA.mode == RoutingMode::incremental)
        fatal(Errors::ExecutionError, "incremental routing not implemented");

    vector<int> non_det_flip;       // log of available non-det flips (-1 = none so need int)
    non_det_flip.reserve(n);        // over-estimated space but fine
    vector<size_t> current_alg = source_alg;// alg. rep of current node

    size_t m;                       // number of bit-flips
    size_t q;                       // a particular double-bit
    size_t p;                       // a particular double-bit
    size_t num_dbits = (n + 1) / 2; // number of dbits in an alg. rep.
    size_t the_bit;                 // a bit
    int non_det_bit;                // a non-det bit (-1 = no non-det choice so need int)
    size_t a_size_t;                // a size_t
    pair<size_t, size_t> source_dbit_p;// a dbit = pair of bits
    pair<size_t, size_t> dest_dbit_p;// a dbit = pair of bits
    string the_entry;               // a table entry
    int counter;                    // counter for num non-det guesses made ('RA.nondet_repeat' is int)
    size_t the_index;               // an index in a list
    bool done;

    map<pair<pair<size_t, size_t>, pair<size_t, size_t> >, string> the_table;// the instruction table
    build_orig_table(the_table);    // build the table

    if ((RA.mode == RoutingMode::nondet) || (RA.mode == RoutingMode::det))
    {
        counter = 0;
        while (counter < RA.nondet_repeat)
        {
            route.port_route.clear();// initialize route
            route.error_flag = {Errors::Good, 0};
            current_alg = source_alg;// set source-alg
            done = false;
            while (!done)           // loops until we reach destination (which we haven't at the start)
            {
                max_double_bit(current_alg, destination_alg, num_dbits, q, m);// get 'q' and 'm'
                if (m == 2)
                {
                    a_size_t = 0;
                    if (RA.mode == RoutingMode::nondet)
                        a_size_t = RNG_routing.getSizeT(0, 1);// make a guess of lq or rq
                    if (a_size_t == 0)
                        the_bit = 2 * q + 1;// this is the det-mode choice
                    else
                        the_bit = 2 * q;
                }
                else if (m == 1)
                {
                    if (current_alg[2 * q] != destination_alg[2 * q])// get the bit that is different
                        the_bit = 2 * q;
                    else
                        the_bit = 2 * q + 1;
                    p = max_non_twiddles(current_alg, destination_alg, q);// get next dbit 'p' to left where
                    if (p != q)                                           //  source-dest. not-twiddles
                    {
                        source_dbit_p = {current_alg[2 * p], current_alg[2 * p + 1]};
                        dest_dbit_p = {destination_alg[2 * p], destination_alg[2 * p + 1]};
                        the_entry = the_table[{source_dbit_p, dest_dbit_p}];// consult the table
                        if (the_entry == "l")
                            the_bit = 2 * p;// flip the left bit of dbit 'p'
                        else if (the_entry == "rq")
                        {
                            a_size_t = 0;
                            if (RA.mode == RoutingMode::nondet)
                                a_size_t = RNG_routing.getSizeT(0, 1);// make a guess of rp or 'the_bit'
                            if (a_size_t == 0)
                                the_bit = 2 * p + 1;// flip the right bit of dbit 'p' (the det-mode choice)
                        }
                    }
                }
                else                // 'm' = 0
                    done = true;    // current = dest so jump out of while-loop
                if (m > 0)          // if yet to finish extend route and no fault
                {
                    if ((FAU.fault_check == FaultCheck::interim_en_route)
                        || (FAU.fault_check == FaultCheck::interim_en_route_and_post))
                    {
                        size_t curr_id;
                        error = crossedcube_alg_convert_to_id(G.pr, current_alg, curr_id);
                        bool chan_health = int_net[curr_id].node_get_port_health(System::out, the_bit);
                        size_t neigh_id = int_net[curr_id].node_get_port_node(System::out, the_bit);
                        bool neigh_health = int_net[neigh_id].node_get_health();
                        if (chan_health == G.faulty)
                        {
                            done = true;
                            route.error_flag = {Errors::FaultyChannel, route.port_route.size()};
                        }
                        else if (neigh_health == G.faulty)
                        {
                            done = true;
                            route.error_flag = {Errors::FaultyNode, route.port_route.size() + 1};
                        }
                    }
                    route.port_route.push_back(the_bit);// extend the route
                    flip_bit(the_bit, current_alg);// build the alg id of node just added to the route
                }
            }                       // we will definitely jump out of this while-loop
            ++route.times_tried;    // register the route-build
            if ((FAU.fault_check == FaultCheck::interim_at_end)
                || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
                route.error_flag = fault_check_route(int_net, source, destination, route);// fault check
            if (route.error_flag.code == Errors::Good)// if its a healthy one then return
                return;
            counter++;              // increment the number of bad runs
            if (RA.mode == RoutingMode::det)// if det mode then stop
                counter = RA.nondet_repeat;
        }                           // if no route found (here) then return last bad route
    }
    else if (RA.mode == RoutingMode::full)
    {
        route.port_route.clear();// initialize route
        route.error_flag = {Errors::Good, 0};

        if (FAU.fault_check == FaultCheck::interim_en_route)
        {
            if (int_net[source].node_get_health() == G.faulty)
            {
                route.error_flag = {Errors::FaultySource, 0};
                ++route.times_tried;
                return;
            }
        }

        current_alg = source_alg;   // so can assume source is healthy when fault-check mode
                                    //  is "interim_en_route"
        #ifdef DEBUG_MODE
        std::cout << "\nstart with\n";
        std::cout << "source      = " << vector_to_a_string(current_alg, 0) << " (" << source << ")\n";
        std::cout << "destination = " << vector_to_a_string(destination_alg, 0) << " ("
            << destination << ")\n";
        std::cin.get();
        #endif // DEBUG_MODE

        while (true)                // do loop until source = dest or fault found (we break out)
        {
            #ifdef DEBUG_MODE
            std::cout << "in main while-loop\n\n";
            #endif // DEBUG_MODE

            bool inner_done = false;
            while (!inner_done)     // this while-loop extends a route to the destination or until a fault
            {
                max_double_bit(current_alg, destination_alg, num_dbits, q, m);// get 'q' and 'm'

                #ifdef DEBUG_MODE
                std::cout << "max_double_bit in minor while-loop says q = " << q << " and m = " << m << "\n";
                #endif // DEBUG_MODE

                if (m == 2)
                {
                    the_bit = 2 * q + 1;// do the right-hand bit first
                    non_det_bit = static_cast<int>(2 * q);// with the left-hand bit as the non-det choice

                    #ifdef DEBUG_MODE
                    std::cout << "the_bit = " << the_bit << " and non_det_bit = " << non_det_bit << "\n";
                    #endif // DEBUG_MODE
                }
                else if (m == 1)
                {
                    if (current_alg[2 * q] == destination_alg[2 * q])// get the bit that is different
                        the_bit = 2 * q + 1;
                    else
                        the_bit = 2 * q;
                    p = max_non_twiddles(current_alg, destination_alg, q);// get next dbit 'p' to left where

                    #ifdef DEBUG_MODE
                    std::cout << "max_non_twiddles gives p = " << p << "\n";
                    #endif // DEBUG_MODE

                    if (p != q)                                           //  source-dest. not-twiddles
                    {
                        source_dbit_p = {current_alg[2 * p], current_alg[2 * p + 1]};
                        dest_dbit_p = {destination_alg[2 * p], destination_alg[2 * p + 1]};
                        the_entry = the_table[{source_dbit_p, dest_dbit_p}];// consult the table
                        if (the_entry == "l")
                        {
                            the_bit = 2 * p;// flip the left bit of dbit 'p'
                            non_det_bit = -1;// no non-det choice
                        }
                        else if (the_entry == "rq")
                        {
                            non_det_bit = static_cast<int>(2 * p + 1);// set right bit of dbit 'p' as non-
                        }                                             //  det choice; leave 'the_bit'
                        else if (the_entry == "q")
                        {
                            non_det_bit = -1;// no non-det choice with 'the_bit' left alone
                        }
                    }
                    else            // no such 'p'
                        non_det_bit = -1;// no non-det choice with 'the_bit' left alone

                    #ifdef DEBUG_MODE
                    std::cout << "the_bit = " << the_bit << " and non_det_bit = " << non_det_bit << "\n";
                    #endif // DEBUG_MODE
                }
                else
                {
                    inner_done = true;// jump out of minor while-loop as route built

                    #ifdef DEBUG_MODE
                    std::cout << "\nfound a complete route and jumped out of minor while-loop\n";
                    #endif // DEBUG_MODE
                }
                if (m > 0)          // if still building then extend route
                {
                    if ((FAU.fault_check == FaultCheck::interim_en_route)
                        || (FAU.fault_check == FaultCheck::interim_en_route_and_post))
                    {
                        size_t curr_id;
                        error = crossedcube_alg_convert_to_id(G.pr, current_alg, curr_id);
                                                                    // get absid of 'current_alg'
                        bool chan_health = int_net[curr_id].node_get_port_health(System::out, the_bit);
                                                    // get health of channel to potential new node
                        size_t neigh_id = int_net[curr_id].node_get_port_node(System::out, the_bit);
                                                                        // get potential new node
                        bool neigh_health = int_net[neigh_id].node_get_health();
                                                                // get health of potential new node
                        if (chan_health == G.faulty)
                        {
                            route.error_flag = {Errors::FaultyChannel, route.port_route.size()};
                            inner_done = true;

                            #ifdef DEBUG_MODE
                            std::cout << "\nintended bit-flip " << the_bit;
                            std::cout << " but jumped out of minor while-loop because of faulty channel\n";
                            #endif // DEBUG_MODE
                        }
                        else if (neigh_health == G.faulty)
                        {
                            route.error_flag = {Errors::FaultyNode, route.port_route.size() + 1};
                            inner_done = true;

                            #ifdef DEBUG_MODE
                            std::cout << "\nintended bit-flip " << the_bit;
                            std::cout << "but jumped out of minor while-loop because of faulty node\n";
                            #endif // DEBUG_MODE
                        }
                    }

                    route.port_route.push_back(the_bit);// extend the route (irrespective of whether faulty)
                    non_det_flip.push_back(non_det_bit);// extend 'non_det_flip'
                    flip_bit(the_bit, current_alg);// build alg id of node added to the route
                }
                if (!inner_done)
                {
                    #ifdef DEBUG_MODE
                    std::cout << "\nmade bit assignments:\n";
                    state_debug(current_alg, destination_alg, route, non_det_flip);
                    #endif // DEBUG_MODE
                }
                else
                {
                    if (route.error_flag.code != Errors::Good)
                    {
                        #ifdef DEBUG_MODE
                        std::cout << "\nhere is the assignment that would have been made:\n";
                        state_debug(current_alg, destination_alg, route, non_det_flip);
                        #endif // DEBUG_MODE
                    }
                }
            }                       // we will definitely jump out of this while-loop as 'm' becomes 0
                                    //  or we'll have been fault-checking and found a fault
            ++route.times_tried;    // register the route-build
            if ((FAU.fault_check == FaultCheck::interim_at_end)
                || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
                route.error_flag = fault_check_route(int_net, source, destination, route);// fault check
            if (route.error_flag.code == Errors::Good)// if its a healthy one then return
            {
                #ifdef DEBUG_MODE
                std::cout << "\nit's a good route!\n";
                #endif // DEBUG_MODE

                return;
            }

            #ifdef DEBUG_MODE
            std::cout << "but the route is faulty:\n";
            view_a_route(int_net, source, destination, route, std::cout);
            std::cout << "so rewinding ...\n\n";
            #endif // DEBUG_MODE

            done = false;
            the_index = route.port_route.size();// length of route (necessarily > 0)
            while (the_index > 0)
            {
                the_index--;
                if (non_det_flip[the_index] != -1)
                {
                    if ((FAU.fault_check == FaultCheck::interim_en_route)// checking faults
                        || (FAU.fault_check == FaultCheck::interim_en_route_and_post))
                    {
                        size_t new_bit = static_cast<size_t>(non_det_flip[the_index]);// new bit (!= -1)
                        flip_bit(route.port_route[the_index], current_alg);// rewind node (for moment)
                        size_t rewound_id;// abs_id of rewound node
                        error = crossedcube_alg_convert_to_id(G.pr, current_alg, rewound_id);
                        bool chan_health = int_net[rewound_id].node_get_port_health(System::out, new_bit);
                                                                // health of channel to potential new node
                        size_t new_id = int_net[rewound_id].node_get_port_node(System::out, new_bit);
                                                                // potential new node
                        bool new_health = int_net[new_id].node_get_health();
                                                                // health of potential new node
                        if ((chan_health == G.healthy) && (new_health == G.healthy))// if all healthy then
                        {                                                          //  register node found
                            flip_bit(route.port_route[the_index], current_alg);// reset node as move will
                            done = true;                                       //  be made elsewhere
                            break;
                        }
                        else        // new node found but there were faults so rewind
                        {
                            route.port_route.pop_back();// rewind port (node already rewound)
                            non_det_flip.pop_back();// rewind non-det (= unhealthy)

                            #ifdef DEBUG_MODE
                            std::cout << "rewound because non-det move with bit " << new_bit << " is faulty and we are at:\n";
                            state_debug(current_alg, destination_alg, route, non_det_flip);
                            #endif // DEBUG_MODE
                        }
                    }
                    else
                    {
                        done = true;
                        break;
                    }
                }
                else
                {
                    flip_bit(route.port_route[the_index], current_alg);// rewind node
                    route.port_route.pop_back();// rewind port
                    non_det_flip.pop_back();// rewind non-det (= -1)

                    #ifdef DEBUG_MODE
                    std::cout << "rewound:\n";
                    state_debug(current_alg, destination_alg, route, non_det_flip);
                    #endif // DEBUG_MODE
                }
            }
            if (done)               // if we have found an index where 'non-det' != -1 (and healthy)
            {
                #ifdef DEBUG_MODE
                std::cout << "found a non-det bit != -1\n";
                #endif // DEBUG_MODE

                flip_bit(route.port_route[the_index], current_alg);     // replace node with new node
                flip_bit(static_cast<size_t>(non_det_flip[the_index]), current_alg);
                route.port_route.pop_back();                            // replace route-port
                route.port_route.push_back(static_cast<size_t>(non_det_flip[the_index]));
                non_det_flip[the_index] = -1;                           // no guess left and go again
                route.error_flag = {Errors::Good, 0};                   // reset error flag

                #ifdef DEBUG_MODE
                std::cout << "so now we have:\n";
                state_debug(current_alg, destination_alg, route, non_det_flip);
                #endif // DEBUG_MODE
            }
            else
            {
                #ifdef DEBUG_MODE
                std::cout << "out with fail\n";
                #endif // DEBUG_MODE

                return;
            }
        }
    }
    else
        fatal(Errors::NotConAllowable, "there is no 'incremental' routing mode implemented");
}

/**
********************************************************************************************************
********************************************************************************************************
The 'crossedcube_ChangSungHsu' routing algorithm from:
 - C.-P. Chang, T.-Y. Sung, and L.-H. Hsu. A new shortest path routing algorithm and embedding cycles of
   crossed cube, Proc. of Int. Symp. on Parallel Architectures, Algorithms and Networks (I-SPAN), pages
   125–131. IEEE Press, 1997.
The version of this routing algorithm that is cross-referenced is as laid out in I.A. Stewart, Minimal
routing in crossed cubes.
********************************************************************************************************
********************************************************************************************************
**/

/**
First, we give functions that are used in both phase 1 and phase 2.
**/

bool dp(// supplied data:
        const vector<size_t>&       current_alg,
        const vector<size_t>&       destination_alg,
        const size_t                dbit,
        const vector<size_t>&       rho,
        const size_t                num_dbits)
/*
Decide whether 'current_alg' and 'destination_alg' are such that their 'dbit'th d-bits, from d-bits
0, 1, ..., 'num_dbits' - 1, are d.p. w.r.t. 'rho' (see Section III.B). Note that we always call this
function with 'dbit' < max {i : 'rho[i]' > 0} (we don't check this); so, we assume throughout that
d-bit 'dbit' has 2 bits and that 'current_alg' and 'destination_alg' are different. We return true if
the 'dbit'th d-bits are d.p. and false otherwise.

Functions:
 - check_odd            from here
*/
{
    size_t sum = 0;
    for (size_t j = num_dbits; j-- > dbit + 1; )// count 'rho[dbit+1]+...+rho[q]' (no need to work with
    {                           //  max {i : 'rho[i]' > 0} as we can simply work with 'num_dbits - 1')
        sum = sum + rho[j];
    }
    if (current_alg[2 * dbit] == 0)// if 'current_alg[2dbit]' = 0 then twiddling does not affect 'dbit'
    {
        if ((current_alg[2 * dbit] == destination_alg[2 * dbit])
            && (current_alg[2 * dbit + 1] == destination_alg[2 * dbit + 1]))// if dbits are to be d.-p.,
            return true;                                                    //  need them equal
    }
    else                        // current_alg[2'dbit'] = 1 so twiddling does affect 'dbit'
    {
        if (!check_odd(sum))    // if 'rho[dbit+1]+...+rho[q]' is even ...
        {
            if ((current_alg[2 * dbit] == destination_alg[2 * dbit])
                && (current_alg[2 * dbit + 1] == destination_alg[2 * dbit + 1]))// even twiddles doesn't
                return true;                 //  change things so we need the dbits to be equal: 1x = 1x
        }
        else                    // otherwise odd twiddles changes things
        {
            if ((current_alg[2 * dbit] == destination_alg[2 * dbit])
                && (current_alg[2 * dbit + 1] != destination_alg[2 * dbit + 1]))// need right-hand
                return true;                                      //  bits different: 1x = 1bar{x}
        }
    }
    return false;
}

size_t hamming_distance(// supplied data:
                        const vector<size_t>&    current_alg,
                        const vector<size_t>&    destination_alg,
                        const size_t             left_bit,
                        const size_t             right_bit)
/*
Get the Hamming distance between 'current_alg' and 'destination_alg' taking into account only the bits
'left_bit', ..., 'right_bit'. We should only call this function with 'left_bit' <= 'right_bit' which
we don't check (if 'left_bit' > 'right_bit' then 0 is returned).
*/
{
    size_t hamming_distance = 0;
    for(size_t i = left_bit; i <= right_bit; ++i)
    {
        if (current_alg[i] != destination_alg[i])
            hamming_distance++;
    }
    return hamming_distance;
}

size_t max_index_in_vector(// supplied data:
                           const vector<size_t>&    the_vector)
/*
Returns the maximum i for which 'the_vector[i]' > 0. If all 0s then returns 'the_vector.size()'. If
'the_vector' is empty then 0 is returned.
*/
{
    size_t size_of_vector = the_vector.size();
    if (size_of_vector == 0)
        return 0;
    for (size_t i = size_of_vector; i-- > 0; )
    {
        if (the_vector[i] > 0)
            return i;
    }
    return size_of_vector;
}

vector<size_t> compute_rho(// supplied data:
                           const vector<size_t>&    current_alg,
                           const vector<size_t>&    destination_alg,
                           const size_t             num_dbits)
/*
Computes the function 'rho' (see Section III.B) where 'current_alg' is the source node, 'destination_alg'
is the destination node and 'num_dbits' is the number of double bits, i.e., /lceil n/2 /rceil.

Functions:
 - max_double_bit           from here
 - hamming_distance         from here
 - dp                       from here
*/
{
    size_t q;                               // max d-bit where d-bits of 'u' and 'v' are different
    size_t m;                               // number of differing bits for d-bit 'q'
    vector<size_t> rho;                     // the function 'rho'
    rho.resize(num_dbits, 0);               // reserve space for 'rho' and initialize at 0

    max_double_bit(current_alg, destination_alg, num_dbits, q, m);// get max dbit data
    if (m == 0)                             // 'current_alg' = 'destination_alg' so return 'rho' = 0
        return rho;
    if ((q == num_dbits - 1) && (check_odd(n)))// if max dbit = 1 bit only (the (n-1)th)
        rho[q] = hamming_distance(current_alg, destination_alg, n - 1, n - 1);
    else                                    // else max dbit = 2 bits
        rho[q] = hamming_distance(current_alg, destination_alg, 2 * q, 2 * q + 1);
    for (size_t i = q; i-- > 0; )           // compute rho[i], i = q-1, q-2, ..., 0
    {
        rho[i] = 1;                         // provisionally set rho[i] = 1
        if (dp(current_alg, destination_alg, i, rho, num_dbits))
            rho[i] = 0;
    }
    return rho;
}

bool comp_right(// supplied data:
                const vector<size_t>&    destination_alg,
                const size_t             dbit,
                const vector<size_t>&    rho,
                const size_t             num_dbits,
                // supplied/computed data:
                vector<size_t>&          current_alg)
/*
Decide whether 'current_alg' and 'destination_alg' are such that their 'dbit'th d-bits, out of
d-bits 0, 1, ..., 'num_dbits' - 1, are comp.-d.p. w.r.t. to the function 'rho' and where the right bit is
complemented (see Section III.B). No checking is done as regards the values 'dbit' and 'num_dbits'. In
particular, we should not call this function with 'dbit' equal to 'num_dbits' - 1 (so we assume throughout
that d-bit 'dbit' has 2 bits).

Functions:
 - dp           from here
*/
{
    bool a_bool;
    current_alg[2 * dbit + 1] = 1 - current_alg[2 * dbit + 1];// flip bit 2'dbit'+1 of 'current_alg'
    a_bool = dp(current_alg, destination_alg, dbit, rho, num_dbits);// test for d.p.
    current_alg[2 * dbit + 1] = 1 - current_alg[2 * dbit + 1];// re-flip bit 2'dbit'+1 of 'current_alg'
    return a_bool;
}

bool comp_left(// supplied data:
               const vector<size_t>&    destination_alg,
               const size_t             dbit,
               const vector<size_t>&    rho,
               const size_t             num_dbits,
               // computed data:
               vector<size_t>&          current_alg)
/*
Decide whether 'current_alg' and 'destination_alg' are such that their 'dbit'th d-bits, out of
d-bits 0, 1, ..., 'num_dbits' - 1, are comp.-d.p. w.r.t. to the function 'rho' and where the left bit is
complemented (see Section III.B). No checking is done as regards the values 'dbit' and 'num_dbits'. In
particular, we should not call this function with 'dbit' equal to 'num_dbits' - 1 (so we assume throughout
that d-bit 'dbit' has 2 bits).

Functions:
 - dp           from here
*/
{
    bool a_bool;
    current_alg[2 * dbit] = 1 - current_alg[2 * dbit];// flip bit 2'dbit' of 'current_alg'
    a_bool = dp(current_alg, destination_alg, dbit, rho, num_dbits);// test for d.p.
    current_alg[2 * dbit] = 1 - current_alg[2 * dbit];// re-flip bit 2'dbit' of 'current_alg'
    return a_bool;
}

void OSR_full(// supplied data:
              const vector<size_t>&    destination_alg,
              const size_t             num_dbits,
              const size_t             j,
              // computed data:
              size_t&                  d,
              vector<size_t>&          non_det_bits,
              vector<size_t>&          Q,
              vector<size_t>&          remove_Q,
              vector<size_t>&          current_alg,
              vector<size_t>&          rho)
/*
This function undertakes the body of OSR in "full" mode with parameter 'j'. So, it computes 'd' and
'd_non_det' and:
 - returns 'd'
 - registers 'd_non_det' in 'non_det_bits', with n denoting that there is no non-det choice
 - if there is no non-det bit choice then 'j' is removed from 'Q' which is registered in 'remove_Q'
 - if there is a non-det bit choice then 'Q' is unchanged and n is registered in 'remove_Q'
 - 'current_alg' is amended by flipping bit 'd'
 - 'rho[j]' is reduced by 1.
Note that we assume that 'j' has already been registered in 'OSR_dbits'. This function applies in
both phase 1 and phase 2.

Functions:
 - check_odd            from here
 - comp_left            from here
 - comp_right           from here
 - flip_bit             from here
*/
{
    size_t d_non_det;
    if (rho[j] == 2)
    {
        d = 2 * j;                      // try bit 2'j' first
        d_non_det = 2 * j + 1;          // remember non-guessed bit
        remove_Q.push_back(n);          // register no change to 'Q'
    }
    else                                // so 'rho[j]' = 1
    {
        d_non_det = n;                  // set that there is no non-det bit
        if ((j != num_dbits - 1) || (!check_odd(n)))// check dbit 'j' has 2 bits
        {
            if (comp_left(destination_alg, j, rho, num_dbits, current_alg))// left-comp-d.p. test
                d = 2 * j;
            else if (comp_right(destination_alg, j, rho, num_dbits, current_alg))// right comp-d.p. test
                d = 2 * j + 1;
            else
            {
                d = 2 * j;              // choose the bit where they differ
                if (current_alg[2 * j + 1] != destination_alg[2 * j + 1])
                    d = 2 * j + 1;
            }
        }
        else
            d = 2 * j;                  // choose the last bit
        Q[j] = 0;                       // take 'j' from 'Q'
        remove_Q.push_back(j);          // register change to 'Q'
    }
    flip_bit(d, current_alg);           // set 'current_alg' = u'
    --rho[j];                           // note that 'rho[j]' was >= 1
    non_det_bits.push_back(d_non_det);  // register non-det bit
}

void OSR_nondet(// supplied data:
                const vector<size_t>&    destination_alg,
                const size_t             num_dbits,
                const size_t             j,
                // computed data:
                size_t&                  d,
                vector<size_t>&          Q,
                vector<size_t>&          current_alg,
                vector<size_t>&          rho)
/*
This function undertakes the body of OSR in "nondet" or "det" mode with parameter 'j'. In "nondet" mode,
it computes 'd' and:
 - returns 'd'
 - if there is no non-det bit choice then 'j' is removed from 'Q'
 - if there is a non-det bit choice then 'Q' is unchanged
 - 'current_alg' is amended by flipping bit 'd'
 - 'rho[j]' is reduced by 1.
In "det" mode we always choose the largest when a non-det choice. This function applies in both phase 1
and phase 2.

Functions:
 - check_odd            from here
 - comp_left            from here
 - comp_right           from here
 - flip_bit             from here
*/
{
    if (rho[j] == 2)                    // so 'rho[j]' = 2 ...
    {
        if (RA.mode == RoutingMode::nondet)
            d = RNG_routing.getSizeT(2 * j, 2 * j + 1);// randomly choose d
        else
            d = 2 * j + 1;
    }
    else                                // so 'rho[j]' = 1 ...
    {
        if ((j != num_dbits - 1) || (!check_odd(n)))// check dbit 'j' has 2 bits
        {
            if (comp_left(destination_alg, j, rho, num_dbits, current_alg))// left-comp-d.p. test
                d = 2 * j;
            else if (comp_right(destination_alg, j, rho, num_dbits, current_alg))// right comp-d.p. test
                d = 2 * j + 1;
            else
            {
                d = 2 * j;              // choose the bit where they differ
                if (current_alg[2 * j + 1] != destination_alg[2 * j + 1])
                    d = 2 * j + 1;
            }
        }
        else
            d = 2 * j;                  // choose the last bit
        Q[j] = 0;                       // take 'j' from 'Q'
    }
    flip_bit(d, current_alg);           // set 'current_alg' = u'
    --rho[j];                           // note that 'rho[j]' was >= 1
}

void amend_OSR_bit_move_and_register(// supplied data:
                                     const struct RouteStruct&    route,
                                     // computed data:
                                     vector<size_t>&              current_alg,
                                     vector<size_t>&              non_det_bits)
/*
Amend and register a move using a non-nil non-det bit in OSR. This means amending:
 - 'current_alg'        flip old last bit and then flip new non-det bit (from 'non_det_bits')
 - 'non_det_bits'       replace new non-det bit entry with n
Note that in OSR, if there is a non-det choice then 'Q' is never amended and 'rho' is always
decremented, with the d-bit j for both the flipped bit and the non-det bit being identical. So
there is no need to change registration data re: 'Q' and 'remove_Q' nor to change 'rho'. This
function applies in both phase 1 and phase 2. Note that we still need to amend the route and
that we have already noted the new non-det bit (before over-writing here).

Functions:
 - flip_bit         from here
*/
{
    size_t the_index = route.port_route.size() - 1;         // the index of the last entry
    size_t d = route.port_route[the_index];                 // previous bit that was flipped
    flip_bit(d, current_alg);                               // go to previous node
    size_t new_d = non_det_bits[the_index];                 // the non-det bit
    flip_bit(new_d, current_alg);                           // go to non-det bit-flip node
    non_det_bits[the_index] = n;                            // de-register non-det bit
}

/**
Phase 1 stuff (lines 6-17 in Alg. 2)
**/

size_t precompute_j_phase_1(//supplied data:
                            vector<size_t>&          current_alg,
                            const vector<size_t>&    destination_alg,
                            const vector<size_t>&    Q,
                            const vector<size_t>&    rho,
                            const size_t             num_dbits)
/*
Precompute 'j from lines 13-15 of Alg. 2 for OSR in line 16. Note that if 'Q' is non-empty then max_j below
is always valid. However, when 'Q' is empty, max_j is n. The return from this function determines whether the
while-loop in line 12 is done or not.

Functions:
 - max_index_in_vector          from here
 - comp_right                   from here
 - comp_left                    from here
*/
{
    size_t j = max_index_in_vector(Q);  // j = max j s.t. j in Q (or n if 'Q' is empty)
    if (j == Q.size())                  // 'Q.size()' is returned if 'Q' is empty
        return n;
    for (size_t i = 0; i < j; ++i)
    {
        if (Q[i] == 1)
            if (comp_right(destination_alg, i, rho, num_dbits, current_alg)
                || comp_left(destination_alg, i, rho, num_dbits, current_alg))
                return i;
    }
    return j;                           // 'j' is input to OSR
}

/**
Phase 2 stuff (lines 19-28 in Alg. 2)
**/

void precompute_i_j_phase_2(// supplied data:
                            const vector<size_t>&    Q,
                            const size_t             num_dbits,
                            // computed data:
                            size_t&                  i,
                            size_t&                  j)
/*
Precompute 'i' and 'j from lines 20-21 of Alg. 2:
 - 'j' = max {j : j in Q}
 - 'i' = max {i : i in Q, i < j}
Note that if 'j' = n on return then 'Q' is empty. If 'j' != n and 'i' = n then 'Q' is a singleton.
*/
{
    i = n;
    j = n;
    for (size_t k = 0; k < num_dbits; ++k)
    {
        if (Q[k] == 1)
        {
            i = j;
            j = k;
        }
    }
}

void get_j_for_OSR_phase_2(// supplied data:
                           const vector<size_t>&    destination_alg,
                           vector<size_t>&          current_alg,
                           const vector<size_t>&    rho,
                           const size_t             num_dbits,
                           const size_t             i,
                           const size_t             j,
                           // computed data:
                           size_t&                  dbit,
                           size_t&                  non_det_dbit)
/*
Compute the value 'dbit' as input for OSR (named j in OSR) as well as any alternative non-det dbit
'non_det_dbit' from line 25 of Alg. 2, with 'non_det_dbit' = n if there is no alternative non-det dbit
(since the call to OSR is from line 23 or 27 of Alg. 2). The values for 'i' and 'j' have already been
computed from lines 20-21 via precompute_i_j_phase_2(). We assume that 'j' != n (which we don't check).

Functions:
 - comp_left            from here
 - comp_right           from here
*/
{
    non_det_dbit = n;               // default is no non-det dbit
    if ((rho[j] == 1) && (i != n) && (comp_left(destination_alg, i, rho, num_dbits, current_alg)))
        dbit = i;                                                               // line 22 of Alg. 2
    else if ((rho[j] == 1) && (i != n) && (comp_right(destination_alg, i, rho, num_dbits, current_alg)))
    {                                                                           // line 24 of Alg. 2
        dbit = i;
        non_det_dbit = j;
    }
    else
        dbit = j;
}

void register_dbits_from_Alg2_phase_2(// supplied data:
                                      const size_t       dbit,
                                      const size_t       non_det_dbit,
                                      // supplied/computed data:
                                      vector<size_t>&    OSR_dbits,
                                      vector<size_t>&    OSR_non_det_dbits)
/*
Registers 'dbit' and 'non_det_dbit', as found by get_j_for_OSR_phase_2(), in 'OSR_dbits' and
'OSR_non_det_dbits'
*/
{
    OSR_dbits.push_back(dbit);
    OSR_non_det_dbits.push_back(non_det_dbit);
}

void rewind_prior_to_non_det_dbit_OSR_move_phase_2(// supplied/computed data:
                                                   RouteStruct&       route,
                                                   vector<size_t>&    current_alg,
                                                   vector<size_t>&    Q,
                                                   vector<size_t>&    remove_Q,
                                                   vector<size_t>&    rho,
                                                   vector<size_t>&    non_det_bits,
                                                   vector<size_t>&    OSR_dbits,
                                                   vector<size_t>&    OSR_non_det_dbits)
/*
After a failed call or double call to OSR in line 25 of Alg. 2:
 - get the last bit 'd' flipped from 'route.port_route'
 - remove 'd' from 'route.port_route'
 - get the dbit 'j' = last bit in 'OSR_dbits'
 - reset 'Q[j]' using 'remove_Q'
 - add 1 to 'rho[j]'
 - return 'current_alg' to previous node by flipping bit 'd'
 - strip last values from 'remove_Q' and 'non_det_bits'
 - set last bit of 'OSR_dbits' to last bit of 'OSR_non_det_dbits'
 - set last value of 'OSR_non_det_dbits' to n

Functions:
 - flip_bit         from here
*/
{
    size_t the_index = route.port_route.size() - 1;
    size_t d = route.port_route[the_index];
    route.port_route.pop_back();
    size_t j = OSR_dbits[the_index];
    if (remove_Q[the_index] == j)
        Q[j] = 1;
    ++rho[j];
    flip_bit(d, current_alg);
    remove_Q.pop_back();
    non_det_bits.pop_back();
    OSR_dbits[the_index] = OSR_non_det_dbits[the_index];
    OSR_non_det_dbits[the_index] = n;
}

/**
**********************************
The phase 1 and phase 2 functions.
**********************************
**/

#ifdef DEBUG_MODE
void state_phase_1(// supplied data:
                   const vector<size_t>&        current_alg,
                   const vector<size_t>&        destination_alg,
                   const struct RouteStruct&    route,
                   const vector<size_t>&        rho,
                   const vector<size_t>&        Q,
                   const vector<size_t>&        remove_Q,
                   const vector<size_t>&        non_det_bits,
                   const vector<size_t>&        OSR_dbits,
                   const size_t                 length)
/*
Print the state during phase 1 and wait for the user to hit return.

Functions:
 - vector_to_a_string           from misc_formatting_functions
*/
{
    [[maybe_unused]] Errors error;
    size_t rel_id;

    error = crossedcube_alg_convert_to_id(G.pr,current_alg,rel_id);
    std::cout << "current_alg      = " << vector_to_a_string(current_alg, 0) << " (abs_id " << rel_id << ")\n";
    error = crossedcube_alg_convert_to_id(G.pr,destination_alg,rel_id);
    std::cout << "destination_alg  = " << vector_to_a_string(destination_alg, 0) << " (abs_id " << rel_id << ")\n";
    std::cout << "route.port_route = " << vector_to_a_string(route.port_route, 0) << "\n";
    std::cout << "rho              = " << vector_to_a_string(rho, 0) << "\n";
    std::cout << "Q                = " << vector_to_a_string(Q, 0) << "\n";
    std::cout << "remove_Q         = " << vector_to_a_string(remove_Q, 0) << "\n";
    std::cout << "non_det_bits     = " << vector_to_a_string(non_det_bits, 0) << "\n";
    std::cout << "OSR_dbits        = " << vector_to_a_string(OSR_dbits, 0) << "\n";
    std::cout << "route length     = " << length << "\n\n";
    std::cin.get();
}

void state_phase_2(// supplied data:
                   const vector<size_t>&        current_alg,
                   const vector<size_t>&        destination_alg,
                   const struct RouteStruct&    route,
                   const vector<size_t>&        rho,
                   const vector<size_t>&        Q,
                   const vector<size_t>&        remove_Q,
                   const vector<size_t>&        non_det_bits,
                   const vector<size_t>&        OSR_dbits,
                   const vector<size_t>&        OSR_non_det_dbits,
                   const size_t                 length)
/*
Print the state during phase 2 and wait for the user to hit return.
*/
{
    [[maybe_unused]] Errors error;
    size_t rel_id;

    error = crossedcube_alg_convert_to_id(G.pr,current_alg,rel_id);
    std::cout << "current_alg       = " << vector_to_a_string(current_alg, 0) << " (abs_id " << rel_id << ")\n";
    error = crossedcube_alg_convert_to_id(G.pr,destination_alg,rel_id);
    std::cout << "destination_alg   = " << vector_to_a_string(destination_alg, 0) << " (abs_id " << rel_id << ")\n";
    std::cout << "route.port_route  = " << vector_to_a_string(route.port_route, 0) << "\n";
    std::cout << "rho               = " << vector_to_a_string(rho, 0) << "\n";
    std::cout << "Q                 = " << vector_to_a_string(Q, 0) << "\n";
    std::cout << "remove_Q          = " << vector_to_a_string(remove_Q, 0) << "\n";
    std::cout << "non_det_bits      = " << vector_to_a_string(non_det_bits, 0) << "\n";
    std::cout << "OSR_dbits         = " << vector_to_a_string(OSR_dbits, 0) << "\n";
    std::cout << "OSR_non_det_dbits = " << vector_to_a_string(OSR_non_det_dbits, 0) << "\n";
    std::cout << "route length      = " << length << "\n\n";
    std::cin.get();
}
#endif // DEBUG_MODE

bool phase_1(// supplied data:
             vector<node>&          int_net,
             const size_t           source,
             const size_t           destination,
             vector<size_t>&        destination_alg,
             const size_t           num_dbits,
             //computed data:
             struct RouteStruct&    route,
             vector<size_t>&        current_alg,
             vector<size_t>&        T,
             vector<size_t>&        Q,
             vector<size_t>&        remove_Q,
             vector<size_t>&        rho,
             vector<size_t>&        non_det_bits,
             vector<size_t>&        OSR_dbits)
/**
Undertakes the computation from lines 8-17 of Alg. 2. There are:
 - 'Q'                  set of dbits defined in Alg. 2
 - 'rho'                function on dbits defined in the paper in Section III.B
and the route is built in
 - 'route.port.route'.
At any time, the current node is held in:
 - 'current_alg'
and the route is extended immediately after a call to OSR in line 16. At this point,
the length of the route is identical to the sizes of the following registers (lifo queues):
 - 'remove_Q'           dbits removed from Q in line 13 of OSR with n denoting no change
 - 'non_det_bits'       non-det bit values d' in line 2 of OSR with n denoting no non-det-bit alternative
 - 'OSR_dbits'          dbits j for each call made to OSR.
For any index k, the kth port on 'route.port_route' and kth values of 'remove_Q', 'non_det_bits' and
'OSR_dbits' all correspond (to a specific execution of OSR). We never call this function with the source
equal to the destination, i.e., on entry, 'Q' is non-empty.

Functions:
 - state_phase_1                            from here
 - precompute_j_phase_1                     from here
 - OSR_full                                 from here
 - fault_check_route                        from int_net_construction
 - amend_OSR_bit_move_and_register          from here
**/
{
    size_t length = 0;          // size of registered data/current route

    #ifdef DEBUG_MODE
    std::cout << "\non entry to phase 1 with state:\n";
    state_phase_1(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                  OSR_dbits, length);
    #endif // DEBUG_MODE

    size_t j;                   // an input to OSR
    size_t d;                   // the value returned from OSR

    vector<size_t> T_indices;   // indices t where 'T[t]' = 1
    T_indices.reserve(num_dbits);
    for (size_t t = 0; t < T.size(); ++t)
    {
        if (T[t] == 1)
            T_indices.push_back(t);
    }

    bool first_run = true;      // denotation of first run

    while (true)                // while there are things to do ...
    {
        if (length == 0)        // if current size of registered data/current route is 0 ...
        {
            if (!T_indices.empty())     // if 'T' is non-empty then choose j in 'T'
            {
                #ifdef DEBUG_MODE
                std::cout << "T = " << vector_to_a_string(T, 0) << " is non-empty\n";
                #endif // DEBUG_MODE

                j = RNG_routing.getSizeT(0, T_indices.size() - 1);// randomly choose a j from 'T'
                j = T_indices[j];
                T[j] = 0;       // remove j from 'T' and 'T_indices'
                T_indices.erase(std::remove(T_indices.begin(), T_indices.end(), j), T_indices.end());

                #ifdef DEBUG_MODE
                std::cout << "chosen j = " << j << " from T\n";
                #endif // DEBUG_MODE
            }
            else
            {
                if (!first_run)// if 'T' is empty and not the first run then fail
                    return false;
                else
                {
                    j = precompute_j_phase_1(current_alg, destination_alg, Q, rho, num_dbits);// j from lines
                        //  13-15 of Alg. 2 (if n then line 12 while-loop done, i.e., 'Q' is empty): if j = n
                        //  then we need to fault-check; otherwise we have the j for the call in line 16

                    #ifdef DEBUG_MODE
                    std::cout << "precompute_j_phase_1 returns dbit j = " << j << "\n";;
                    #endif // DEBUG_MODE
                }
            }
            first_run = false;  // we've handled the first run
        }
        else
        {
            j = precompute_j_phase_1(current_alg, destination_alg, Q, rho, num_dbits);// j from lines
                    //  13-15 of Alg. 2 (if n then line 12 while-loop done, i.e., 'Q' is empty): if j = n
                    //  then we need to fault-check; otherwise we have the j for the call in line 16

                #ifdef DEBUG_MODE
                std::cout << "precompute_j_phase_1 returns dbit j = " << j << "\n";;
                #endif // DEBUG_MODE
        }

        if (j != n)             // do the OSR call
        {
            OSR_dbits.push_back(j);// register 'j'
            OSR_full(destination_alg, num_dbits, j, d, non_det_bits, Q, remove_Q, current_alg, rho);
                // get bit 'd' to flip (to get u') and non-det-d with 'Q' and 'rho' amended and
                //  'current_alg' updated
            route.port_route.push_back(d);// extend the route found (line 17 in Alg. 2)
            ++length;           // extend the size of registered data/current route

            #ifdef DEBUG_MODE
            std::cout << "\ndone OSR with state:\n";
            state_phase_1(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                          OSR_dbits, length);
            #endif // DEBUG_MODE

        }                       // if we are here then we have extended the route so loop again
        else                    // we have a source-destination route and so need to fault-check

        {
            ++route.times_tried;// register the route-build
            route.error_flag = {Errors::Good, 0};// assume route healthy (note route is non-empty)
            if ((FAU.fault_check == FaultCheck::interim_at_end)
                || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
                route.error_flag = fault_check_route(int_net, source, destination, route);// fault-check
            if (route.error_flag.code == Errors::Good)
            {
                #ifdef DEBUG_MODE
                std::cout << "Q is empty and found a good route and so done\n";
                #endif // DEBUG_MODE

                return true;    // if route is good then done
            }
            else                // if route is faulty then need to back-track (note route is non-empty)
            {
                #ifdef DEBUG_MODE
                std::cout << "Q is empty but found a bad route\n";
                #endif // DEBUG_MODE

                size_t new_d = non_det_bits[length - 1];// get the possible new 'd'
                if (new_d != n) // if there is a non-det-bit-move then do it
                {
                    amend_OSR_bit_move_and_register(route, current_alg, non_det_bits);
                        // replace the most recent move with the non-det-bit-move: note that we do not
                        //  have to amend 'rho', 'Q', 'OSR-dbits' or 'remove_Q'
                    route.port_route[length - 1] = new_d;// reset the port on the tour to the new 'd'

                    #ifdef DEBUG_MODE
                    std::cout << "however there was a non-det bit-move and state is now:\n";
                    state_phase_1(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                  OSR_dbits, length);
                    #endif // DEBUG_MODE

                }               // we now need to loop again
                else            // if we are here then there was no non-det-bit-move so rewind route
                {               //  (note that current length of route 'length' > 0)

                    #ifdef DEBUG_MODE
                    std::cout << "no non-det bit-move so we need to rewind\n";
                    #endif // DEBUG_MODE

                    bool rewind = true;
                    while (rewind)
                    {
                        // rewind current state

                        size_t d = route.port_route[length - 1];
                        size_t j = OSR_dbits[length - 1];
                        flip_bit(d, current_alg);
                        ++rho[j];
                        if (remove_Q[length - 1] == j)
                            Q[j] = 1;
                        route.port_route.pop_back();
                        OSR_dbits.pop_back();
                        remove_Q.pop_back();
                        non_det_bits.pop_back();
                        --length;

                        #ifdef DEBUG_MODE
                        std::cout << "we've rewound once and the state is now:\n";
                        state_phase_1(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                      OSR_dbits, length);
                        #endif // DEBUG_MODE

                        if (length == 0)// if we have rewound all the way then we need to go again
                            break;

                        // go to previous state and see whether it can be rewound

                        new_d = non_det_bits[length - 1];
                        if (new_d != n)// if we can make a non-det-bit-move then do it else keep rewinding
                        {
                            amend_OSR_bit_move_and_register(route, current_alg, non_det_bits);
                                // replace the most current move with the non-det-bit-move: note that we do
                                //  not have to amend 'rho', 'Q', 'OSR-dbits' or 'remove_Q'
                            route.port_route[length - 1] = new_d;// reset port on the tour to the new 'd'
                            rewind = false;// stop rewinding and continue looping

                            #ifdef DEBUG_MODE
                            std::cout << "there was a non-det bit-move and state is now:\n";
                            state_phase_1(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                          OSR_dbits, length);
                            #endif // DEBUG_MODE
                        }
                    }
                }
            }
        }
    }
    return false;               // should never get to here
}

bool phase_2(// supplied data:
             vector<node>&          int_net,
             const size_t           source,
             const size_t           destination,
             vector<size_t>&        destination_alg,
             const size_t           num_dbits,
             //computed data:
             struct RouteStruct&    route,
             vector<size_t>&        current_alg,
             vector<size_t>&        Q,
             vector<size_t>&        remove_Q,
             vector<size_t>&        rho,
             vector<size_t>&        non_det_bits,
             vector<size_t>&        OSR_dbits,
             vector<size_t>&        OSR_non_det_dbits)
/**
Undertakes the computation from lines 20-28 of Alg. 2. There are:
 - 'Q'                  set of dbits defined in Alg. 2
 - 'rho'                function on dbits defined in the paper in Section III.B
and the route is built in
 - 'route.port.route'.
At any time, the current node is held in:
 - 'current_alg'
and the route is extended immediately after a call to OSR (in lines 23, 25 and 27). At this point,
the length of the route is identical to the sizes of the following registers (lifo queues):
 - 'remove_Q'           dbits removed from Q in line 13 of OSR with n denoting no change
 - 'OSR_dbits'          d-bit values i-x-j for the calls to OSR, in line 23, 25 or 27 of Alg. 2
 - 'OSR-non_det_dbits'  non-det d-bit values x for the calls to OSR in line 25 of Alg. 2, with a value
                        of n if the call comes from lines 23 or 27 of Alg. 2
 - 'non_det_bits'       non-det bit values d' in line 2 of OSR with n denoting no non-det-bit alternative.
For any index k, the kth port on 'route.port_route' and kth values of 'remove_Q', 'OSR_dbits',
'OSR-non_det_dbits' and 'non_det_bits' all correspond (to a specific execution of OSR). On entry, the
lengths of these registers are all 0.

Functions called:
 - state_phase_2                                            from here (debugging)
 - precompute_i_j_phase_2                                   from here
 - get_j_for_OSR_phase_2                                    from here
 - register_dbits_from_Alg2_phase_2                         from here
 - OSR_full                                                 from here
 - fault_check_route                                        from int_net_construction
 - amend_OSR_bit_move_and_register                          from here
 - rewind_prior_to_non_det_dbit_OSR_move_phase_2            from here
**/
{
    size_t length = 0;          // current length of registers/route

    #ifdef DEBUG_MODE
    std::cout << "\non entry to phase 2 with state:\n";
    state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits, OSR_dbits,
                  OSR_non_det_dbits, length);
    #endif // DEBUG_MODE

    bool finished = false;      // a bool to denote when phase 2 is finished
    while (!finished)

    {
        size_t i;               // dbit in line 21 of Alg. 2
        size_t j;               // dbit in line 20 of Alg. 2
        size_t dbit;            // dbit for OSR in lines 23, 25 and 27
        size_t non_det_dbit;    // alternative non-det dbit in line 25 of Alg. 2
        size_t d;               // bit returned from OSR

        precompute_i_j_phase_2(Q, num_dbits, i, j);
            // pre-compute i and j from lines 20-21 of Alg. 2 (j = n = not defined)

        #ifdef DEBUG_MODE
        std::cout << "precompute_i_j_phase_2 returns dbit j = " << j << " and dbit i = " << i << "\n";;
        #endif // DEBUG_MODE

        if (j != n)             // if Q is non-empty ...
        {
            get_j_for_OSR_phase_2(destination_alg, current_alg, rho, num_dbits, i, j, dbit, non_det_dbit);
            // get input j = 'dbit' to OSR and any non-det alternative 'non_det_dbit'

            #ifdef DEBUG_MODE
            std::cout << "get_j_for_OSR_phase_2 returns dbit j = " << j << "\n";;
            #endif // DEBUG_MODE

            register_dbits_from_Alg2_phase_2(dbit, non_det_dbit, OSR_dbits, OSR_non_det_dbits);
                // add 'dbit' and 'non_det_dbit' to 'OSR_dbits' and 'OSR_non_det_dbits'
            OSR_full(destination_alg, num_dbits, dbit, d, non_det_bits, Q, remove_Q, current_alg, rho);
                // compute OSR to get 'd' and 'd_non_det' and update 'Q', 'remove_Q', 'rho', 'current_alg'
                //  and 'non_det_bits'
            route.port_route.push_back(d);// extend the route found (line 28 in Alg. 2)
            ++length;

            #ifdef DEBUG_MODE
            std::cout << "\ndone OSR with state:\n";
            state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits, OSR_dbits,
                          OSR_non_det_dbits, length);
            #endif // DEBUG_MODE
        }
        else                    // if Q is empty ...
        {
            ++route.times_tried;// register the route-build
            route.error_flag = {Errors::Good, 0};// assume route healthy (note route is non-empty)
            if ((FAU.fault_check == FaultCheck::interim_at_end)
                || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
                route.error_flag = fault_check_route(int_net, source, destination, route);// fault-check
            if (route.error_flag.code == Errors::Good)
            {
                #ifdef DEBUG_MODE
                std::cout << "Q is empty and found a good route and so done\n";
                #endif // DEBUG_MODE

                return true;    // if route is good then done
            }
            else                // if route is faulty then need to back-track (note route is non-empty)
            {
                #ifdef DEBUG_MODE
                std::cout << "Q is empty but found a bad route\n";
                #endif // DEBUG_MODE

                size_t new_d = non_det_bits[length - 1];// get the possible new 'd'
                size_t new_dbit;
                if (new_d != n) // if there is a non-det-bit-move then do it and loop again
                {
                    amend_OSR_bit_move_and_register(route, current_alg, non_det_bits);
                        // replace the most recent move with the non-det-bit-move: note that we do not
                        //  have to amend 'rho', 'Q', 'OSR-dbits', 'OSR_non_det_dbits' or 'remove_Q'
                    route.port_route[length - 1] = new_d;// reset the port on the tour to the new 'd'

                    #ifdef DEBUG_MODE
                    std::cout << "however there was a non-det bit-move and state is now:\n";
                    state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                  OSR_dbits, OSR_non_det_dbits, length);
                    #endif // DEBUG_MODE
                }
                else if (OSR_non_det_dbits[length - 1] != n)// if there is a non-det dbit move then do it
                {                                           //  and loop again
                    rewind_prior_to_non_det_dbit_OSR_move_phase_2(route, current_alg, Q, remove_Q, rho,
                                                            non_det_bits, OSR_dbits, OSR_non_det_dbits);
                        // rewind last bit-move in OSR, i.e., pop 'route.port_route', rewind 'current_alg',
                        //  amend 'Q' and 'rho', strip 'remove_Q' and 'non_det_bits', and reset 'OSR_dbits'
                        //  and 'OSR_non_det_dbits'

                    OSR_full(destination_alg, num_dbits, dbit, d, non_det_bits, Q, remove_Q, current_alg,
                             rho);// compute OSR to get 'd' and 'd_non_det' and update 'Q', 'remove_Q',
                                  //  'rho', 'current_alg', compute OSR to get 'd' and 'd_non_det' and
                                  // update 'Q', 'remove_Q', 'rho', 'current_alg' and 'non_det_bits'
                    route.port_route.push_back(d);// extend the route found (line 28 in Alg. 2)

                    #ifdef DEBUG_MODE
                    std::cout << "no non-det bit-move but there was a non-det dbit-move and state is now:\n";
                    state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                  OSR_dbits, OSR_non_det_dbits, length);
                    #endif // DEBUG_MODE
                }
                else            // if we are here then there was no non-det bit-move or non-det dbit-move
                {               //  so need to rewind
                    #ifdef DEBUG_MODE
                    std::cout << "no non-det bit-move and no non-det dbit-move so we need to rewind\n";
                    #endif // DEBUG_MODE

                    bool rewind = true;
                    while (rewind)
                    {
                        // rewind current state

                        size_t d = route.port_route[length - 1];
                        size_t j = OSR_dbits[length - 1];
                        flip_bit(d, current_alg);
                        ++rho[j];
                        if (remove_Q[length - 1] == j)
                            Q[j] = 1;
                        route.port_route.pop_back();
                        OSR_dbits.pop_back();
                        OSR_non_det_dbits.pop_back();
                        remove_Q.pop_back();
                        non_det_bits.pop_back();
                        --length;

                        if (length == 0)// if we have rewound all the way then we have failed
                        {
                            finished = true;

                            #ifdef DEBUG_MODE
                            std::cout << "we've rewound all the way and we have failed\n\n";
                            #endif // DEBUG_MODE

                            break;
                        }

                        #ifdef DEBUG_MODE
                        std::cout << "we've rewound once and the state is now:\n";
                        state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                  OSR_dbits, OSR_non_det_dbits, length);
                        #endif // DEBUG_MODE

                        // go to previous state and see whether it can be rewound

                        new_d = non_det_bits[length - 1];
                        new_dbit = OSR_non_det_dbits[length - 1];
                        if (new_d != n)// if we can make a non-det-bit-move then do it
                        {
                            amend_OSR_bit_move_and_register(route, current_alg, non_det_bits);
                                // replace most current move with non-det-bit-move: note that we do not have
                                //  to amend 'rho', 'Q', 'OSR_dbits', 'OSR_non_det_bits'  or 'remove_Q'
                            route.port_route[length - 1] = new_d;// reset port on the tour to the new 'd'
                            rewind = false;// stop rewinding and continue looping

                            #ifdef DEBUG_MODE
                            std::cout << "there was a non-det bit-move and the state is now:\n";
                            state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                  OSR_dbits, OSR_non_det_dbits, length);
                            #endif // DEBUG_MODE
                        }
                        else if (new_dbit != n)
                        {
                            rewind_prior_to_non_det_dbit_OSR_move_phase_2(route, current_alg, Q, remove_Q, rho,
                                                            non_det_bits, OSR_dbits, OSR_non_det_dbits);
                                // rewind last bit-move in OSR, i.e., pop 'route.port_route', rewind
                                //  'current_alg', amend 'Q' and 'rho', strip 'remove_Q' and 'non_det_bits',
                                //  and reset 'OSR_dbits' and 'OSR_non_det_dbits'

                            OSR_full(destination_alg, num_dbits, dbit, d, non_det_bits, Q, remove_Q, current_alg,
                                     rho);// compute OSR to get 'd' and 'd_non_det' and update 'Q', 'remove_Q',
                                          //  'rho', 'current_alg', compute OSR to get 'd' and 'd_non_det' and
                                          // update 'Q', 'remove_Q', 'rho', 'current_alg' and 'non_det_bits'
                            route.port_route.push_back(d);// extend the route found (line 28 in Alg. 2)
                            rewind = false;// stop rewinding and continue looping

                            #ifdef DEBUG_MODE
                            std::cout << "no non-det bit-move but there was a non-det dbit-move and state";
                            std::cout << " is now:\n";
                            state_phase_2(current_alg, destination_alg, route, rho, Q, remove_Q, non_det_bits,
                                          OSR_dbits, OSR_non_det_dbits, length);
                            #endif // DEBUG_MODE
                        }
                        // if get to here then we need to continue rewinding
                    }
                }
            }
        }
    }
    return false;
}

/**
*****************************
The initialization functions.
*****************************
**/

void initialization_phase_2(// supplied data:
                            const vector<size_t>&        current_alg,
                            const vector<size_t>&        destination_alg,
                            const size_t                 num_dbits,
                            // computed data:
                            vector<size_t>&              rho,
                            vector<size_t>&              Q)
/*
Initialize the main data structures 'rho' and 'Q' (space already reserved).

Functions:
 - compute_rho          from here
*/
{
    rho = compute_rho(current_alg, destination_alg, num_dbits); // build 'rho'

    Q.assign(num_dbits, 0);
    for (size_t i = 0; i < num_dbits; ++i)  // build 'Q'
    {
        if (rho[i] > 0)
            Q[i] = 1;
    }
}

void initialization_phase_1(// supplied data:
                            vector<size_t>&          current_alg,
                            const vector<size_t>&    destination_alg,
                            const size_t             num_dbits,
                            // computed data:
                            vector<size_t>&          rho,
                            vector<size_t>&          Q,
                            vector<size_t>&          T)
/*
Initialize the main data structures 'rho', 'Q', 'T' (space already reserved). This function
is only called if source != destination; so 'q' below is always defined.

Functions:
 - initialization_phase_2           from here
 - comp_left                        from here
 - comp_right                       from here
*/
{
    size_t q;
    initialization_phase_2(current_alg, destination_alg, num_dbits, rho, Q);
    for (size_t i = num_dbits; i-- > 0; )
    {
        if (Q[i] == 1)
        {
            q = i;
            break;
        }
    }

    T.assign(num_dbits, 0);
    for (size_t i = 0; i < q; ++i)          // build 'T'
    {
        if (rho[i] > 0)
        {
            if (comp_left(destination_alg, i, rho, num_dbits, current_alg)
                || comp_right(destination_alg, i, rho, num_dbits, current_alg))
                T[i] = 1;
        }
    }
}

/**
********************************************
The main algorithm crossedcube_ChangSungHsu.
********************************************
**/

void crossedcube_ChangSungHsu(// supplied data:
                              vector<node>&           int_net,
                              const size_t            source,
                              const size_t            destination,
                              // computed data:
                              struct RouteStruct&     route)
/**
The improved crossed cube routing algorithm from C.-P. Chang, T.-Y. Sung, and L.-H. Hsu. Edge congestion
and topological properties of crossed cube. IEEE Transactions on Parallel and Distributed Systems,
11(1):64–80, 2000. The algorithm is non-deterministic. When run so that:
 - 'RA.mode' = "nondet", it simply makes some guesses and returns the route (with 'RA.nondet_repeat' bad
                         runs allowed)
 - 'RA.mode' = "det",    it runs with the lowest guess at each stage
 - 'RA.mode' = "full",   it iterates through all possible guesses (in the case of faults).
Fault checking is undertaken within the routing algorithm if 'FAU.fault_check' = "interim_en_route" or
"interim_at_end" so that:
    - if "interim_en_route" then a route is built noting faulty components en route (so no fault-check of
      the final route is required)
    - if "interim_at_end" then a route is built ignoring faulty components (so a fault-check of final
      route is required)
    - if "post" then regardless as to whether faults have been catered for in route building, a
      fault-check is undertaken when the route is returned
    - if "interim_en_route_and_post" then as for "interim_en_route" with an external "post" check too
      (used when debugging)
    - if "interim_at_end_and_post" then as for "interim_at_end" with an external "post" check too
      (used when debugging).
However, "interim_en_route" and "interim_en_route_and_post" are as yet not implemented. No "incremental"
mode has been implemented. Only "full" mode has debugging information.

Functions:
 - crossedcube_alg_convert_from_id          from crossedcube (via alg_from_numeric_ptr)
 - initialization_phase_1                   from here
 - max_index_in_vector                      from here
 - OSR_full                                 from here
 - state_phase_1                            from here (debugging)
 - phase_1                                  from here
 - initialization_phase_2                   from here
 - phase_2                                  from here
 - fatal                                    from error_writing
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
        ++route.times_tried;
        return;
    }

    /** USER-DEFINED **/

    if (RA.mode == RoutingMode::incremental)
        fatal(Errors::ExecutionError, "incremental routing not implemented");
    if ((FAU.fault_check == FaultCheck::interim_en_route)
        || (FAU.fault_check == FaultCheck::interim_en_route_and_post))
        fatal(Errors::ExecutionError, "en route fault-checking not implemented");

    vector<size_t> current_alg;     // alg. rep of current node
    current_alg = source_alg;       // set current = source

    size_t num_dbits = (n + 1) / 2; // number of d-bits in an alg. rep. = ceiling(n/2)

    vector<size_t> rho;             // the function rho from the paper
    rho.reserve(num_dbits);         // initialize space for 'rho'
    vector<size_t> Q;               // the set Q from the paper
    Q.reserve(num_dbits);           // initialize space for 'Q'
    vector<size_t> T;               // the set T from the paper
    T.reserve(num_dbits);           // initialize space for 'T'
    vector<size_t> OSR_dbits;       // the choices made for the input j to OSR
    OSR_dbits.reserve(num_dbits + 1);// initialize space for 'OSR_dbits'
    vector<size_t> OSR_non_det_dbits;// non-det choices for the input j to OSR in line 25 of Alg. 2
    OSR_non_det_dbits.reserve(num_dbits + 1);// initialize space for 'OSR_dbits'
    vector<size_t> non_det_bits;    // non-det choices for bit-move in line 2 of OSR
    non_det_bits.reserve(num_dbits + 1);// initialize space for 'non_det_bits'

    bool success;                   // whether we've found a healthy route (assume we haven't)

    // when 'RA.mode' = full

    if (RA.mode == RoutingMode::full)
    {
        /*
        All non-T non-det choices are from 2 possible choices. So, when 'RA.mode' = full, when we make a
        choice we need to remember the 'choice that wasn't made' for the future. The choices made are
        held in 'route.port_route':
        - c0, c1, ..., ci
        with the shadow choices being held in 'non_det_bits':
        - s0, s1, ..., si
        with sj = n if there is no shadow choice. When things fail we pop ci and si from their queues
        until we find a non-n shadow choice sj which becomes cj with sj = n. Note that the port-list and
        the shadow-list always have the same length. Also, when rewinding, we need to rewind other
        registers too (see phase_1() and phase_2() for full details).
        */

        vector<size_t> non_det_bits;    // register of non-det bits from line 2 of One_Step_Route Alg. 3
        non_det_bits.reserve(num_dbits + 1);// <= 'num_dbits' + 1 possible bit-flips

        /*
        'remove_Q' has size identical to 'route.port_route' and 'non_det_bits'. 'remove_Q' tracks whether j
        was removed from Q in OSR (and has the value of j if so, n otherwise).
        */

        vector<size_t> remove_Q;        // register of changes to 'Q' (by OSR)
        remove_Q.reserve(num_dbits + 1);

        // phase 1 (lines 6-17)

        initialization_phase_1(current_alg, destination_alg, num_dbits, rho, Q, T);// Alg. 2 l. 1, 3, 6-7

        success = phase_1(int_net, source, destination, destination_alg, num_dbits, route,
                                  current_alg, T, Q, remove_Q, rho, non_det_bits, OSR_dbits);
        if (success)
            return;

        // phase 2 (lines 19-28; only if a healthy route has not been found in phase 1)

        current_alg = source_alg;   // set current = source
        route.port_route.clear();   // reinitialize 'route'
        route.error_flag = {Errors::Good, 0};

        initialization_phase_2(current_alg, destination_alg, num_dbits, rho, Q);// Alg. 2 l. 1, 3

        non_det_bits.clear();       // clear registrations
        remove_Q.clear();
        OSR_dbits.clear();
        OSR_non_det_dbits.clear();

        success = phase_2(int_net, source, destination, destination_alg, num_dbits, route, current_alg,
                          Q, remove_Q, rho, non_det_bits, OSR_dbits, OSR_non_det_dbits);
        return;
    }
    else if ((RA.mode == RoutingMode::nondet) || (RA.mode == RoutingMode::det))
    {
        /*
        No need to remember registers from "full" mode.
        */

        size_t i;
        size_t j;
        size_t d;
        size_t dbit;
        size_t non_det_dbit;
        int counter = 0;             // remember how many non-det guesses have been made

        while (counter < RA.nondet_repeat)// if we have guesses left ...
        {
            current_alg = source_alg;// reset 'current_alg'
            route.port_route.clear();// reset route
            size_t coin;
            if (RA.mode == RoutingMode::nondet)
                coin = RNG_routing.getSizeT(1, 2);// randomly choose phase 1 or phase 2
            else
                coin = 1;
            if (coin == 1)          // phase 1
            {
                initialization_phase_1(current_alg, destination_alg, num_dbits, rho, Q, T);// Alg. 2 l. 1, 3, 7

                vector<size_t> T_indices;   // put elements in 'T' in 'T_indicies'
                T_indices.reserve(n);
                for (size_t t = 0; t < T.size(); ++t)
                {
                    if (T[t] == 1)
                        T_indices.push_back(t);
                }

                if (!T_indices.empty())     // if 'T' is non-empty then choose a j from 'T' and do OSR
                {
                    if (RA.mode == RoutingMode::nondet)
                        j = RNG_routing.getSizeT(0, T_indices.size() - 1);// randomly choose a t from 'T'
                    else
                        j = 0;
                    j = T_indices[j];
                    OSR_nondet(destination_alg, num_dbits, j, d, Q, current_alg, rho);// execute OSR
                    route.port_route.push_back(d);// extend the route
                }

                j = precompute_j_phase_1(current_alg, destination_alg, Q, rho, num_dbits);// j from l. 13-15
                    //  of Alg. 2 (if n then line 12 while-loop done, i.e., 'Q' is empty): if j = n then we need
                    //  to fault-check; otherwise we have the j for the call in line 16
                while (j != n)
                {
                    OSR_nondet(destination_alg, num_dbits, j, d, Q, current_alg, rho);// execute OSR
                    route.port_route.push_back(d);// extend the route
                    j = precompute_j_phase_1(current_alg, destination_alg, Q, rho, num_dbits);// j from l. 13-15
                }
            }
            else                    // phase 2
            {
                initialization_phase_2(current_alg, destination_alg, num_dbits, rho, Q);// Alg. 2 l. 1, 3

                precompute_i_j_phase_2(Q, num_dbits, i, j);// pre-compute i and j from lines 20-21 of Alg. 2
                                                           //  (j = n = not defined)
                while (j != n)      // while Q is non-empty ...
                {
                    get_j_for_OSR_phase_2(destination_alg, current_alg, rho, num_dbits, i, j, dbit, non_det_dbit);
                        // get input j = 'dbit' to OSR and any non-det alternative 'non_det_dbit'
                    j = dbit;
                    if (RA.mode == RoutingMode::nondet)
                    {
                        if (non_det_dbit != n)
                        {
                            coin = RNG_routing.getSizeT(0, 1);
                            if (coin == 1)
                                j = non_det_dbit;
                        }
                    }
                    OSR_nondet(destination_alg, num_dbits, j, d, Q, current_alg, rho);// compute OSR to get 'd'
                    route.port_route.push_back(d);// extend the route found
                    precompute_i_j_phase_2(Q, num_dbits, i, j);// pre-compute i and j from lines 20-21 of Alg. 2
                                                               //  (j = n = not defined)
                }
            }
            ++route.times_tried;        // register the route-build
            route.error_flag = {Errors::Good, 0};// assume route healthy (note route is non-empty)
            if ((FAU.fault_check == FaultCheck::interim_at_end)
                || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
                route.error_flag = fault_check_route(int_net, source, destination, route);// fault-check
            else
                return;
            if (route.error_flag.code == Errors::Good)
                return;
            ++counter;
        }
        return;                     // ran out of guesses
    }
    else if (RA.mode == RoutingMode::det)
    {
        /*
        No need to remember registers from "full" mode.
        */

        size_t j;
        size_t d;

        initialization_phase_1(current_alg, destination_alg, num_dbits, rho, Q, T);// Alg. 2 l. 1, 3

        j = n;
        for (size_t t = 0; t < T.size(); ++t)
        {
            if (T[t] == 1)
                j = t;              // 'j' = max t from 'T' or n if 'T' empty
        }

        if (j != n)                 // if 'T' is non-empty then do OSR with max j from 'T'
        {
            OSR_nondet(destination_alg, num_dbits, j, d, Q, current_alg, rho);// execute OSR
            route.port_route.push_back(d);// extend the route
        }

        j = precompute_j_phase_1(current_alg, destination_alg, Q, rho, num_dbits);// j from l. 13-15
            //  of Alg. 2 (if n then line 12 while-loop done, i.e., 'Q' is empty): if j = n then we need
            //  to fault-check; otherwise we have the j for the call in line 16
        while (j != n)
        {
            OSR_nondet(destination_alg, num_dbits, j, d, Q, current_alg, rho);// execute OSR
            route.port_route.push_back(d);// extend the route
            j = precompute_j_phase_1(current_alg, destination_alg, Q, rho, num_dbits);// j from l. 13-15
        }

        ++route.times_tried;        // register the route-build
        route.error_flag = {Errors::Good, 0};// assume route healthy (note route is non-empty)
        if ((FAU.fault_check == FaultCheck::interim_at_end)
            || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
            route.error_flag = fault_check_route(int_net, source, destination, route);// fault-check
        if (route.error_flag.code == Errors::Good)
            return;
        else
            return;                 // det-run failed
    }
    else
        fatal(Errors::NotConAllowable, "there is no 'incremental' routing mode implemented");
}

/**
********************************************************************************************************
********************************************************************************************************
The 'Stewart' routing algorithm from:
 - I.A. Stewart, Minimal routing in crossed cubes.
********************************************************************************************************
********************************************************************************************************
**/

void build_table(// computed data:
            map<tuple<pair<size_t, size_t>, pair<size_t, size_t>, bool>, pair<string, int>>&    the_table)
/*
Build the bit-flip table where the index is <dbit, dbit, bool> with 'false' = "m is even" and 'true' =
"m is odd": see paper. Also, 0/1 in value denotes "after even"/"after odd" and -1 denotes that there are
no restrictions.
*/
{
    the_table[{{0,0}, {0,0}, true}] = {"", -1};
    the_table[{{0,0}, {1,0}, true}] = {"l", 1};
    the_table[{{0,0}, {0,1}, true}] = {"r", -1};
    the_table[{{0,0}, {1,1}, true}] = {"l", 0};
    the_table[{{1,0}, {0,0}, true}] = {"l", 0};
    the_table[{{1,0}, {1,0}, true}] = {"r", -1};
    the_table[{{1,0}, {0,1}, true}] = {"l", 1};
    the_table[{{1,0}, {1,1}, true}] = {"", -1};
    the_table[{{0,1}, {0,0}, true}] = {"r", -1};
    the_table[{{0,1}, {1,0}, true}] = {"l", 0};
    the_table[{{0,1}, {0,1}, true}] = {"", -1};
    the_table[{{0,1}, {1,1}, true}] = {"l", 1};
    the_table[{{1,1}, {0,0}, true}] = {"l", 1};
    the_table[{{1,1}, {1,0}, true}] = {"", -1};
    the_table[{{1,1}, {0,1}, true}] = {"l", 0};
    the_table[{{1,1}, {1,1}, true}] = {"r", -1};

    the_table[{{0,0}, {0,0}, false}] = {"", -1};
    the_table[{{0,0}, {1,0}, false}] = {"l", 0};
    the_table[{{0,0}, {0,1}, false}] = {"r", -1};
    the_table[{{0,0}, {1,1}, false}] = {"l", 1};
    the_table[{{1,0}, {0,0}, false}] = {"l", 0};
    the_table[{{1,0}, {1,0}, false}] = {"", -1};
    the_table[{{1,0}, {0,1}, false}] = {"l", 1};
    the_table[{{1,0}, {1,1}, false}] = {"r", -1};
    the_table[{{0,1}, {0,0}, false}] = {"r", -1};
    the_table[{{0,1}, {1,0}, false}] = {"l", 1};
    the_table[{{0,1}, {0,1}, false}] = {"", -1};
    the_table[{{0,1}, {1,1}, false}] = {"l", 0};
    the_table[{{1,1}, {0,0}, false}] = {"l", 1};
    the_table[{{1,1}, {1,0}, false}] = {"r", -1};
    the_table[{{1,1}, {0,1}, false}] = {"l", 0};
    the_table[{{1,1}, {1,1}, false}] = {"", -1};
}

void crossedcube_Stewart(//supplied data:
                         vector<node>&          int_net,
                         const size_t           source,
                         const size_t           destination,
                         // computed data:
                         struct RouteStruct&    route)
/**
Crossed cube routing as devised by me. The algorithm is non-deterministic. When run so that:
 - 'RA.mode' = "nondet", it simply makes some guesses and returns the route (with 'RA.nondet_repeat' bad
                         runs allowed)
 - 'RA.mode' = "det",    it runs with the lowest guess at each stage
 - 'RA.mode' = "full",   it iterates through all possible guesses (in the case of faults).
Fault checking is undertaken within the routing algorithm if 'FAU.fault_check' = "interim_en_route" or
"interim_at_end" so that:
    - if "interim_en_route" then a route is built noting faulty components en route (so no fault-check of
      the final route is required)
    - if "interim_at_end" then a route is built ignoring faulty components (so a fault-check of final
      route is required)
    - if "post" then regardless as to whether faults have been catered for in route building, a
      fault-check is undertaken when the route is returned
    - if "interim_en_route_and_post" then as for "interim_en_route" with an external "post" check too
      (used when debugging)
    - if "interim_at_end_and_post" then as for "interim_at_end" with an external "post" check too
      (used when debugging).
However, "interim_en_route" and "interim_en_route_and_post" are as yet not implemented. No "incremental"
mode has been implemented. Only "full" mode has debugging information.

Functions called:
 - crossedcube_alg_convert_from_id          from crossedcube (via alg_from_numeric_ptr)
 - build_table                              from here
 - max_double_bit                           from here
 - check_odd                                from here
 - RNG_routing.getInt                       from random_gen
 - fault_check_route                        from node_port_related
 - fatal                                    from error_writing
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
        ++route.times_tried;
        return;
    }

    /** USER-DEFINED **/

    if (RA.mode == RoutingMode::incremental)
        fatal(Errors::ExecutionError, "incremental routing not implemented");

    vector<size_t> current_alg;             // alg. rep of current node
    current_alg = source_alg;

    size_t m = 0;                           // iterative number of bit-flips
    size_t q;                               // a particular double-bit
    vector<tuple<string, size_t, int>> flips;// flip-list: <l,r,lr; dbit; 0/1 even/odd,-1 ignore>
    size_t num_dbits = (n + 1) / 2;         // number of dbits in an alg. rep.
    flips.reserve(num_dbits + 1);           // reserve dbits + 1 space (account for maybe "lr")
    pair<string, int> table_entry;          // a table entry
    pair<size_t, size_t> source_pair;       // a source pair of bits
    pair<size_t, size_t> destination_pair;  // a destination pair of bits
    bool m_parity;                          // the parity of 'm' (even = false, odd = true)

    map<tuple<pair<size_t, size_t>, pair<size_t, size_t>, bool>, pair<string, int>> the_table;
                                            // instruction table (bool: false/true = even/odd 'm'
    build_table(the_table);                 // build the table

    // coding of shortest paths computation

    max_double_bit(current_alg, destination_alg, num_dbits, q, m);// max. dbit 'q' where source/dest. differ and number
    if (m == 2)                                                   //  'm' of diff. bits ( 'm' > 0 as source != dest)
        flips.emplace_back("lr", q, -1);    // register "lr" flip
    else if (m == 1)
    {
        if (current_alg[2 * q] != destination_alg[2 * q])// bit 2q = left-most bit of qth d-bit (even if n odd)
            flips.emplace_back("l", q, -1); // register flip
        else
            flips.emplace_back("r", q, -1); // register flip
    }
    for (size_t i = q; i-- > 0; )           // for each decreasing double-bit in turn ...
    {
        m_parity = check_odd(m);            // get the parity of 'm' ('false' = even, 'true' = odd)
        source_pair = {current_alg[2 * i], current_alg[2 * i + 1]};// set the source pair
        destination_pair = {destination_alg[2 * i], destination_alg[2 * i + 1]};// set the destination pair
        table_entry = the_table[{source_pair, destination_pair, m_parity}];// get the instruction from the table
        if (std::get<0>(table_entry) != "")    // if there is something to do ...
        {
            flips.emplace_back(std::get<0>(table_entry), i, std::get<1>(table_entry));// register what is to be done
            m++;                            // increment 'm' (always by 1)
        }
    }

    // end of computation of coding of shortest paths as 'flips': note that 'm' = length of a shortest path > 0

    size_t num_flips = flips.size();    // number of flips in 'flips' (note that there might be + 1 bit-flips with "lr")
    size_t the_index;                   // index of flip being dealt with in 'flips'
    size_t num_bitflips;                // number of bit-flips generated so far
    pair<string, size_t> a_bitflip;     // a bit-flip
    pair<string, size_t> another_bitflip;// another bit-flip
    vector<pair<string, size_t>> path;  // 'path' is the actual bit-flips made in a particular path
    path.reserve(num_flips + 1);        // reserve space for 'path'
    int counter;                        // counter for non-det. tries so far (compare with 'RA.nondet_repeat' (int))
    size_t a_guess;
    size_t a_bit;
    vector<size_t> lexico_range;        // the individual range-values when generating bit-flip paths lexicographically
    lexico_range.reserve(num_flips + 1);// reserve space for 'lexico_range'
    vector<size_t> lexico_start;        // the starting sequence range-values
    lexico_start.reserve(num_flips + 1);// reserve space for 'lexico_start'
    vector<size_t> jump;                // the increments for each flip (depends on after even, after odd, not bothered)
    jump.reserve(num_flips + 1);        // reserve space for 'jump'

    // standard non-det. implementation ("nondet") where we guess a route (or maybe more than one depending upon
    // 'RA.nondet_repeat') with "det" denoting the lexicographic first

    if ((RA.mode == RoutingMode::nondet) || (RA.mode == RoutingMode::det))// guess a route
    {
        counter = 0;                    // count the number of attempts made so far
        while (counter < RA.nondet_repeat)// if we find a healthy route then we'll simply return
        {
            path.clear();               // clear 'path'
            route.port_route.clear();   // clear port-list (but never touch 'route.source')
            route.error_flag = {Errors::Good, 0};// assume good route
            the_index = 0;
            num_bitflips = 0;
            if (get<0>(flips[0]) == "lr")// if the first entry of the 0th flip is s.t. m = 2 ...
            {                            //  (first entry always exists as source != dest)
                a_bitflip = make_pair("l", get<1>(flips[0]));// build <'l', dbit>
                another_bitflip = make_pair("r", get<1>(flips[0]));// build <'r', dbit>
                if (RA.mode == RoutingMode::nondet)
                    a_guess = RNG_routing.getSizeT(0, 1);// guess in {0,1}
                else
                    a_guess = 0;
                if (a_guess == 0)       // if 0 then <'l', dbit>, <'r', dbit>
                {
                    path.push_back(a_bitflip);
                    path.push_back(another_bitflip);
                }
                else                    // if 1 then <'r', i>, <'i', i>
                {
                    path.push_back(another_bitflip);
                    path.push_back(a_bitflip);
                }
                the_index = 1;          // move to next entry in 'flips'
                num_bitflips = 2;       // register number of bit-flips generated
            }
            for (size_t i = the_index; i < num_flips; ++i)// for every (other) flip in 'flips' ...
            {
                if (get<2>(flips[i]) == -1)// no restriction so need int in {0, 1, ..., 'num_bitflips'}
                {
                    if (RA.mode == RoutingMode::nondet)
                        a_guess = RNG_routing.getSizeT(0, num_bitflips);// insert "before, between or after"
                    else
                        a_guess = 0;
                }
                else if (get<2>(flips[i]) == 0)// locate after even so int in {0, 2, ..., 2('num_bitflips'/2)}
                {
                    if (RA.mode == RoutingMode::nondet)
                        a_guess = 2 * RNG_routing.getSizeT(0, num_bitflips / 2);
                    else
                        a_guess = 0;
                }
                else if (get<2>(flips[i]) == 1)// locate after odd so int in {1, 3, ..., 2(('num_bitflips'-1)/2)+1}
                {
                    if (RA.mode == RoutingMode::nondet)
                        a_guess = 2 * RNG_routing.getSizeT(0, (num_bitflips - 1) / 2) + 1;
                    else
                        a_guess = 1;
                }
                a_bitflip = make_pair(get<0>(flips[i]), get<1>(flips[i]));// make bit-flip = ('l', dbit) or ('r', dbit)
                path.insert(path.begin() + a_guess, a_bitflip);// insert bit-flip in 'path' at the guessed location
                num_bitflips++;         // signal one more bit-flip generated
            }

            // now build the path

            for (size_t i = 0; i < num_bitflips; ++i)// for every bit-flip ...
            {
                a_bit = 2 * get<1>(path[i]);// get the actual bit to be flipped
                if (get<0>(path[i]) == "r")
                    a_bit++;
                route.port_route.push_back(a_bit);// register the bit flipped, i.e., output port
            }                                     // note that we don't need to compute the nodes on the route

            // now check the route

            ++route.times_tried;        // register the route-build
            if (FAU.fault_check == FaultCheck::interim_at_end)
                route.error_flag = fault_check_route(int_net, source, destination, route);// check route
            if (route.error_flag.code == Errors::Good)
                return;                 // healthy valid route found so return
            counter++;                  // maybe try another route
        }
        return;                         // simply return the last faulty route
    }
    else if (RA.mode == RoutingMode::full)// check all possible sequences of bit-flips
    {
        #ifdef DEBUG_MODE
        std::cout << "\nsource_alg      = " << vector_to_a_string(source_alg, 0) << "\n";
        std::cout << "destination_alg = " << vector_to_a_string(destination_alg, 0) << "\n";
        std::cout << "\nflips: \n";
        for (size_t i = 0; i < flips.size(); ++i)
        {
            std::cout << "  (" << std::get<0>(flips[i]) << ", " << std::get<1>(flips[i]) << ", "
                << std::get<2>(flips[i]) << ")\n";
        }
        std::cin.get();
        #endif // DEBUG_MODE

        if (get<0>(flips[0]) == "lr")   // if the first entry of the 0th flip is s.t. m = 2 ...
        {
            get<0>(flips[0]) = "r";     // amend first flip of 'flips' to <'l', i, -1>, <'r', i, -1>
            flips.insert(flips.begin(), make_tuple("l", get<1>(flips[0]), get<2>(flips[0])));

            #ifdef DEBUG_MODE
            std::cout << "flips amended: \n";
            for (size_t i = 0; i < flips.size(); ++i)
            {
                std::cout << "  (" << std::get<0>(flips[i]) << ", " << std::get<1>(flips[i]) << ", "
                    << std::get<2>(flips[i]) << ")\n";
            }
            std::cin.get();
            #endif // DEBUG_MODE
        }

        num_bitflips = flips.size();    // number of flips to make
        for (size_t i = 0; i < num_bitflips; ++i)// build the data structures to generate all flip orders
        {                                        //  'lexico_range' = [0, place to put flip-1, place to put flip-2, ...]
            if ((get<2>(flips[i]) == 0) || (get<2>(flips[i]) == -1))// if "after even" or "ignore"
            {
                lexico_start.push_back(0);// start at 0
                lexico_range.push_back(0);// first place-to-put is 0
            }
            else                        // if "after odd"
            {
                lexico_start.push_back(1);// start at 1
                lexico_range.push_back(1);// first place-to-put is 1
            }
            if ((get<2>(flips[i]) == 0) || (get<2>(flips[i]) == 1)) // if "after even" or "after odd"
                jump.push_back(2);      // increment by 2
            else                        // if "ignore"
                jump.push_back(1);      // increment by 1
        }                               // note that 'lexico_range' is now starting places-to-put

        #ifdef DEBUG_MODE
        std::cout << "lexico_start: [";
        for (size_t i = 0; i < num_bitflips; ++i)
        {
            std::cout << lexico_start[i];
            if (i < num_bitflips - 1)
                std::cout << ", ";
            else
                std::cout << "]\n";
        }
        std::cout << "lexico_range: [";
        for (size_t i = 0; i < num_bitflips; ++i)
        {
            std::cout << lexico_range[i];
            if (i < num_bitflips - 1)
                std::cout << ", ";
            else
                std::cout << "]\n";
        }
        std::cout << "jump        : [";
        for (size_t i = 0; i < num_bitflips; ++i)
        {
            std::cout << jump[i];
            if (i < num_bitflips - 1)
                std::cout << ", ";
            else
                std::cout << "]\n";
        }
        std::cin.get();
        #endif // DEBUG_MODE

        the_index = num_bitflips;       // set this so as to enter the while-loop (note: > 0)
        while (the_index > 0)           // keep on generating flips-orders until final one is found
        {                               //  namely when 'the_index' reaches 0
            path.clear();               // clear the order of bit-flips
            for (size_t i = 0; i < num_bitflips; ++i)// for every bit-flip, insert into 'path'
            {                                        //  note that component i ranges between 0 and i
                path.insert(path.begin() + lexico_range[i], make_pair(get<0>(flips[i]), get<1>(flips[i])));
            }
            route.port_route.clear();   // reset route
            for (size_t i = 0; i < num_bitflips; ++i)// for every bit-flip ...
            {
                a_bit = 2 * get<1>(path[i]);// get the actual bit to be flipped
                if (get<0>(path[i]) == "r")
                    a_bit++;
                route.port_route.push_back(a_bit);// register the bit flipped, i.e., output port
            }

            #ifdef DEBUG_MODE
            std::cout << "path: ";
            for (size_t i = 0; i < num_bitflips; ++i)
            {
                std::cout << "<" << get<0>(path[i]) << "," << get<1>(path[i]) << ">";
                if (i < num_bitflips - 1)
                    std::cout << ", ";
                else
                    std::cout << "\n";
            }
            std::cout << "port_route: [";
            for (size_t i = 0; i < num_bitflips; ++i)
            {
                std::cout << route.port_route[i];
                if (i < num_bitflips - 1)
                    std::cout << ", ";
                else
                    std::cout << "]\n";
            }
            std::cin.get();
            #endif // DEBUG_MODE

            ++route.times_tried;        // register the route-build
            route.error_flag = {Errors::Good, 0};// we now have the assumed-good route
            if (FAU.fault_check == FaultCheck::interim_at_end)
                route.error_flag = fault_check_route(int_net, source, destination, route);// error-check route
            if (route.error_flag.code == Errors::Good)
            {
                #ifdef DEBUG_MODE
                std::cout << "healthy route so return\n";
                std::cin.get();
                #endif // DEBUG_MODE

                return;                 // if a healthy route has been found we are done
            }
            else
            {
                #ifdef DEBUG_MODE
                std::cout << "faulty route so try next\n";
                std::cin.get();
                #endif // DEBUG_MODE

                the_index = num_bitflips - 1;// generate the next 'lexico_range' (>= 0)
                while (the_index > 0)   // while we don't reach 0 (when we have finished)
                {
                    if ((lexico_range[the_index] + jump[the_index]) <= the_index)
                        break;          // we've found a component to increment
                    else
                        the_index--;    // try the next component (to the left)
                }
                if (the_index != 0)     // if we haven't reached 0 then increment component
                {
                    lexico_range[the_index] += jump[the_index];// increment component
                    for (size_t i = the_index + 1; i < num_bitflips; ++i)// reset all components to the right
                    {
                        lexico_range[i] = lexico_start[i];
                    }

                    #ifdef DEBUG_MODE
                    std::cout << "lexico_range: [";
                    for (size_t i = 0; i < num_bitflips; ++i)
                    {
                        std::cout << lexico_range[i];
                        if (i < num_bitflips - 1)
                            std::cout << ", ";
                        else
                            std::cout << "]\n";
                    }
                    std::cin.get();
                    #endif // DEBUG_MODE
                }
                else
                {
                    #ifdef DEBUG_MODE
                    std::cout << "done and failed to find a route\n";
                    std::cin.get();
                    #endif // DEBUG_MODE
                }
            }                           // if 'the_index' = 0 then we jump out of main while-loop
        }                               //  otherwise we go again
        return;                         //  a bad route (error-flag is for the last bad route)
    }
    else
        fatal(Errors::NotConAllowable, "there is no 'incremental' routing mode implemented");
}

/**
********************************************************************************************************
********************************************************************************************************
The 'crossedcube_OtakeMouriKanedo_1' routing algorithm from:
 - K. Otake, K. Mouri and K. Kanedo, Fault-tolerant routing methods in crossed cubes, Proc. of 10th Int.
   Conf. on Advances in Information Technology (1918) article 10.
The algorithm as presented in the paper is non-deterministic and it is only suggested that it is run in
this mode, i.e., by arbitrarily choosing some member of Fwd, Swd or Bwd as appropriate.
********************************************************************************************************
********************************************************************************************************
**/

size_t get_distance(// supplied data:
                    const size_t                     start_node,
                    const size_t                     end_node)
/*
Returns the distance from 'start' to 'end'.

Functions called:
 - crossedcube_alg_convert_from_id          from crossedcube
 - compute_rho                              from here
*/
{
    [[maybe_unused]] Errors error;

    vector<size_t> start_alg;
    start_alg.reserve(n);
    vector<size_t> end_alg;
    end_alg.reserve(n);
    error = crossedcube_alg_convert_from_id(G.pr, start_node, start_alg);
    error = crossedcube_alg_convert_from_id(G.pr, end_node, end_alg);
    vector<size_t> rho;
    size_t num_dbits = (n + 1) / 2;
    rho = compute_rho(start_alg, end_alg, num_dbits);
    size_t length = 0;
    for (size_t i = 0; i < num_dbits; ++i)
    {
        length += rho[i];
    }
    return length;
}

void build_Fwd_Swd_Bwd(// supplied data:
                       const vector<node>&                  int_net,
                       const size_t                         curr,
                       const size_t                         dest,
                       const vector<size_t>&                used,
                       // computed data:
                       vector<pair<size_t, size_t>>&        Fwd,
                       vector<pair<size_t, size_t>>&        Swd,
                       vector<pair<size_t, size_t>>&        Bwd)
/*
Build 'Fwd', 'Swd' and 'Bwd' w.r.t. 'curr'. Note that we take into account faults when building Fwd, Swd
and Bwd.

Functions:
 - get_distance         from here
*/
{
    size_t curr_ports = int_net[curr].node_get_num_ports();// number of ports of 'curr'
    size_t dist_curr_dest = get_distance(curr, dest);// get distance from 'curr' to 'dest'
    size_t dist_neigh_dest;
    for (size_t i = 0; i < curr_ports; ++i)// for every neighbour ...
    {
        if (int_net[curr].node_get_port_health(System::out, i) == G.healthy)
        {
            size_t neigh = int_net[curr].node_get_port_node(System::out, i);// get absid of neighbour i
            if (int_net[neigh].node_get_health() == G.healthy)
            {
                if (std::find(used.begin(), used.end(), neigh) == used.end())// if 'neigh' unused so far
                {
                    dist_neigh_dest = get_distance(neigh, dest);// distance from 'neigh' to 'dest'
                    if (dist_neigh_dest < dist_curr_dest)// locate in Fwd, Swd or Bwd
                        Fwd.push_back(make_pair(neigh, i));// pair of form <'neigh', port from 'curr')
                    else if(dist_neigh_dest > dist_curr_dest)
                        Bwd.push_back(make_pair(neigh, i));
                    else
                        Swd.push_back(make_pair(neigh, i));
                }
            }
        }
    }
}

size_t get_Fwd_Swd_Bkd(// supplied/computed data:
                       vector<pair<size_t, size_t>>&    FSBwd,
                       struct RouteStruct&              route,
                       vector<size_t>&                  used)
/*
Choose neighbour from 'Fwd', 'Swd' or 'Bwd'; erase it from the set; add it to 'route'; mark it as used;
and return it. We assume that 'FSBwd.size()' > 0.
*/
{
    size_t index = 0;
    if (RA.mode != RoutingMode::det)
        index = RNG_routing.getSizeT(0, FSBwd.size() - 1);// guess an index
    size_t neigh = FSBwd[index].first; // set neighbour
    size_t port = FSBwd[index].second; // set port to neighbour
    FSBwd.erase(FSBwd.begin() + index);// remove it from 'FSBwd'
    route.port_route.push_back(port);// extend 'route'
    used.push_back(neigh);// signify that neighbour is now used
    return neigh;
}

bool depth_first_search(// supplied data:
                        const vector<node>&    int_net,
                        size_t                 tail,
                        size_t                 destination,
                        // supplied/computed data:
                        size_t&                depth,
                        vector<size_t>&        used,
                        struct RouteStruct&    route)
/*
Undertake a depth-first search from 'tail' looking for 'destination' so that when 'depth' becomes 0 we stop
searching. Return true if 'destination' is found and false otherwise. In the case of a return of true,
'route' holds the overall route, i.e., the route that was held prior to the call to 'depth_first_search'
+ the subsequent added-on channels. The dfs works by searching neighbours in ascending order. Alos, the
dfs is fault-tolerant when searching for 'destination'.

Functions called:
 -
*/
{
    if (tail == destination)        // if 'tail' = 'destination' then we are at 'destination'
        return true;
    if (depth == 0)                 // if we can't go deeper then quit
        return false;
    bool success;
    size_t neigh;
    size_t num_ports = int_net[tail].node_get_num_ports();
    for (size_t i = 0; i < num_ports; ++i)// for every neighbour ...
    {
        neigh = int_net[tail].node_get_port_node(System::out, i);
        if (std::find(used.begin(), used.end(), neigh) == used.end())// if 'neigh' unused so far
        {
            if ((int_net[neigh].node_get_health() == G.healthy)// check that 'neigh' is healthy as is
                && (int_net[tail].node_get_port_health(System::out, i) == G.healthy))//  channel to 'neigh'
            {
                route.port_route.push_back(i);// visit 'neigh'
                --depth;            // note that 'depth' > 0
                used.push_back(neigh);// register 'neigh' as used
                success = depth_first_search(int_net, neigh, destination, depth, used, route);// recurse
                if (success == true)
                    return true;
                else
                {
                    route.port_route.pop_back();// rewind
                    ++depth;
                    used.pop_back();// de-register node as used
                }
            }
        }
    }
    return false;                   // exhausted all neighbours - fail
}

bool method_1_and_2(// supplied data:
                    const vector<node>&    int_net,
                    const size_t           source,
                    const size_t           destination,
                    const size_t           curr,
                    const size_t           method,
                    // computed data:
                    vector<size_t>&        used,
                    struct RouteStruct&    route)
/**
The main recursive algorithm, for "full", "nondet" or "det" modes, so that:
 - if 'FAU.fault_check' = "post" then the algorithm builds the first path and then checks for faults
   on return (without fault-checking internally)
 - if 'FAU.fault_check' = "interim_at_end" then the algorithm pays no heed to whether components are
   healthy or faulty when building and internally fault-checks when the destination is reached or we
   run out of nodes to visit.
The fault-check "interim_en_route" is not implemented. The parameter 'method' specifies whether we
execute "method1" or "method2" from the paper.
**/
{
    if (curr == destination)        // reached the destination so return
    {
        ++route.times_tried;        // register route just built
        if ((FAU.fault_check == FaultCheck::interim_at_end)
            || (FAU.fault_check == FaultCheck::interim_at_end_and_post))
        {
            route.error_flag = fault_check_route(int_net, source, destination, route);// fault check
            if (route.error_flag.code == Errors::Good)
                return true;
            else
                return false;
        }
        return true;
    }

    vector<pair<size_t, size_t>> Fwd;// forward nodes from 'curr'
    Fwd.reserve(n);
    vector<pair<size_t, size_t>> Swd;// sideways nodes from 'curr'
    Swd.reserve(n);
    vector<pair<size_t, size_t>> Bwd;// backward nodes from 'curr'
    Bwd.reserve(n);

    build_Fwd_Swd_Bwd(int_net, curr, destination, used, Fwd, Swd, Bwd);// build the node sets

    if (method == 2)
    {
        size_t depth = get_distance(curr, destination);
        if (depth == 3)
        {
            bool success = depth_first_search(int_net, curr, destination, depth, used, route);
            if (success)
                return true;
        }
    }

    while (Fwd.size() != 0)     // try Fwd nodes
    {
        size_t neigh = get_Fwd_Swd_Bkd(Fwd, route, used);// 'neigh' is added to 'route' + 'used' and
                            //  removed from 'Fwd' ('used' always consists of nodes used on 'route')
        bool success = method_1_and_2(int_net, source, destination, neigh, method, used, route);// recurse
        if (success)            // if destination has been found then accept
            return true;
        else                    // if destination has not been found
        {
            if (RA.mode == RoutingMode::full)
            {
                route.port_route.pop_back();// remove route extension
                used.pop_back();    // 'neigh' is now usable again but not choosable
            }
            else                // in det or nondet mode then simply reject
                return false;
        }
    }
    while (Swd.size() != 0)     // try Swd nodes
    {
        size_t neigh = get_Fwd_Swd_Bkd(Swd, route, used);// 'neigh' is added to 'route' + 'used'
                                                         //  and removed from 'Swd'
        bool success = method_1_and_2(int_net, source, destination, neigh, method, used, route);// recurse
        if (success)            // if destination has been found then accept
            return true;
        else                    // if destination has not been found
        {
            if (RA.mode == RoutingMode::full)
            {
                route.port_route.pop_back();// remove route extension
                used.pop_back();    // 'neigh' is now usable again but not choosable
            }
            else                // in det or nondet mode then simply reject
                return false;
        }
    }
    while (Bwd.size() != 0)     // try Bwd nodes
    {
        size_t neigh = get_Fwd_Swd_Bkd(Bwd, route, used);// 'neigh' is added to 'route' + 'used'
                                                         //  and removed from 'Bwd'
        bool success = method_1_and_2(int_net, source, destination, neigh, method, used, route);// recurse
        if (success)            // if destination has been found then accept
            return true;
        else                    // if destination has not been found
        {
            if (RA.mode == RoutingMode::full)
            {
                route.port_route.pop_back();// remove route extension
                used.pop_back();    // 'neigh' is now usable again but not choosable
            }
            else                // in det or nondet mode then simply reject
                return false;
        }
    }
    ++route.times_tried;        // register route just built
    route.error_flag.code = Errors::BadDestination;
    return false;               // there is nowhere to go from 'curr'
}

void crossedcube_OtakeMouriKanedo_1(//supplied data:
                                    vector<node>&          int_net,
                                    const size_t           source,
                                    const size_t           destination,
                                    // computed data:
                                    struct RouteStruct&    route)
/**
This is the recursive algorithm 'method1' from K. Otake, K. Mouri and K. Kanedo, Fault-tolerant routing
methods in crossed cubes, Proc. of 10th Int. Conf. on Advances in Information Technology (1918) article 10.
My implementation is fault-tolerant in that I check for faults internally after having built a route. When
run so that:
 - 'RA.mode' = "nondet", it simply makes some guesses and returns the route (with 'RA.nondet_repeat' bad
                         runs allowed)
 - 'RA.mode' = "det",    it runs with the first guess at each stage
 - 'RA.mode' = "full",   it iterates through all possible guesses (in the case of faults).
Fault checking:
 - is undertaken within the routing algorithm if 'FAU.fault_check' = "interim_at_end"
 - is undertaken externally if 'FAU.fault_check' = "post", which is essentially equivalent to "det".
No "incremental" mode has been implemented and nor has "interim_en_route" fault-checking.

Note that it makes sense to immediately check for a faulty source but we don't do this. The reason is so
that we can set every node faulty and count the number of paths attempted. This will validate any
closed-form count we might come up with for this number in future.

Note that the algorithm uses no properties of the crossed cube and essentially the same algorithm can be
devised for any interconnection network: all that is needed is a function to compute the length of a
shortest path joining any two nodes.

The "full" recursive algorithm works as follows with u the last node on the route:
 all nodes encountered on the route have been marked as used
 at u, compute:
  - Fwd[u] = {all unused neighbours closer to dest than u is}
  - Swd[u] = {all unused neighbours at the same distance to dest as u is}
  - Bwd[u] = {all unused neighbours further from dest than u is}
 while (Fwd[u] is non-empty)
    choose a node v in Fwd[u]
    mark v as used
    add v to the route
    bool = recurse(v, used, route)
    if (bool)
        return true
    else
        remove v from route
        mark v as unused
 while (Swd[u] is non-empty)
    exactly as above
 while (Bwd[u] is non-empty)
    exactly as above
 return false

Regardless of fault-checking mode, we take faults into account when building Fwd, Swd and Bwd.

Functions called:
 - crossedcube_alg_convert_from_id          from crossedcube (via alg_from_numeric_ptr)
 - coding                                   from error_writing
 - fatal                                    from error_writing
 - method_1                                 from here
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
            || (FAU.fault_check == FaultCheck::interim_at_end))
        {
            if (int_net[source].node_get_health() != G.healthy)
                route.error_flag = {Errors::FaultySource, 0};
        }
        return;
    }

    /** USER-DEFINED **/

    if (RA.mode == RoutingMode::incremental)
        fatal(Errors::ExecutionError, "incremental routing not implemented");
    if ((FAU.fault_check == FaultCheck::interim_en_route)
        || (FAU.fault_check == FaultCheck::interim_en_route_and_post))
        fatal(Errors::ExecutionError, "interim_en_route fault-checking not implemented");

    vector<size_t> used;
    used.reserve(G.total_nodes);
    used.push_back(source);     // register 'source' as used

    if (RA.mode == RoutingMode::full)
    {
        [[maybe_unused]] bool a_bool;
        a_bool = method_1_and_2(int_net, source, destination, source, 1, used, route);
    }
    else if ((RA.mode == RoutingMode::nondet) || (RA.mode == RoutingMode::det))
    {
        [[maybe_unused]] bool a_bool;
        int counter = 0;
        while (counter < RA.nondet_repeat)
        {
            a_bool = method_1_and_2(int_net, source, destination, source, 1, used, route);
            if (a_bool == true)
                return;
            ++counter;
            if (RA.mode == RoutingMode::det)
                counter = RA.nondet_repeat;
        }
    }
}

/**
********************************************************************************************************
********************************************************************************************************
The 'crossedcube_OtakeMouriKanedo_2' routing algorithm from:
 - K. Otake, K. Mouri and K. Kanedo, Fault-tolerant routing methods in crossed cubes, Proc. of 10th Int.
   Conf. on Advances in Information Technology (1918) article 10.
The algorithm as presented in the paper is non-deterministic and it is only suggested that it is run in
this mode, i.e., by arbitrarily choosing some member of Fwd, Swd or Bwd as appropriate.
********************************************************************************************************
********************************************************************************************************
**/

void crossedcube_OtakeMouriKanedo_2(//supplied data:
                                    vector<node>&          int_net,
                                    const size_t           source,
                                    const size_t           destination,
                                    // computed data:
                                    struct RouteStruct&    route)
/**
This is the recursive algorithm 'method2' from K. Otake, K. Mouri and K. Kanedo, Fault-tolerant routing
methods in crossed cubes, Proc. of 10th Int. Conf. on Advances in Information Technology (1918) article 10.
My implementation is fault-tolerant in that I check for faults internally after having built the route. When
run so that:
 - 'RA.mode' = "nondet", it simply makes some guesses and returns the route (with 'RA.nondet_repeat' bad
                         runs allowed)
 - 'RA.mode' = "det",    it runs with the first guess at each stage
 - 'RA.mode' = "full",   it iterates through all possible guesses (in the case of faults).
Fault checking:
 - is undertaken within the routing algorithm if 'FAU.fault_check' = "interim_at_end"
 - is undertaken externally if 'FAU.fault_check' = "post", which is essentially equivalent to "det".
No "incremental" mode has been implemented and nor has "interim_en_route" fault-checking.

Note that it makes sense to immediately check for a faulty source but we don't do this. The reason is so
that we can set every node faulty and count the number of paths attempted. This will validate any
closed-form count we might come up with for this number in future.

Note that the algorithm uses no properties of the crossed cube and essentially the same algorithm can be
devised for any interconnection network: all that is needed is a function to compute the length of a
shortest path joining any two nodes.

It is essentially the same as method_1() and crossedcube_OtakeMouriKanedo_1() from that paper (see above)
except that when a search is distance 3 from the destination, a dfs is undertaken to build a route to the
destination, if possible (given possible faults).

Functions called:
 - crossedcube_alg_convert_from_id          from crossedcube (via alg_from_numeric_ptr)
 - coding                                   from error_writing
 - fatal                                    from error_writing
 - method_2                                 from here
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
            || (FAU.fault_check == FaultCheck::interim_at_end))
        {
            if (int_net[source].node_get_health() != G.healthy)
                route.error_flag = {Errors::FaultySource, 0};
        }
        return;
    }

    /** USER-DEFINED **/

    if (RA.mode == RoutingMode::incremental)
        fatal(Errors::ExecutionError, "incremental routing not implemented");
    if ((FAU.fault_check == FaultCheck::interim_en_route)
        || (FAU.fault_check == FaultCheck::interim_en_route_and_post))
        fatal(Errors::ExecutionError, "interim_en_route fault-checking not implemented");

    vector<size_t> used;
    used.reserve(G.total_nodes);
    used.push_back(source);     // register 'source' as used

    if (RA.mode == RoutingMode::full)
    {
        [[maybe_unused]] bool a_bool;
        a_bool = method_1_and_2(int_net, source, destination, source, 2, used, route);
    }
    else if ((RA.mode == RoutingMode::nondet) || (RA.mode == RoutingMode::det))
    {
        [[maybe_unused]] bool a_bool;
        int counter = 0;
        while (counter < RA.nondet_repeat)
        {
            a_bool = method_1_and_2(int_net, source, destination, source, 2, used, route);
            if (a_bool == true)
                return;
            ++counter;
            if (RA.mode == RoutingMode::det)
                counter = RA.nondet_repeat;
        }
    }
}


