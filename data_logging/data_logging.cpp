
#include "data_logging.h"
#include "../globals.h"
#include "../structs/structs_system.h"

using std::vector;
using std::string;
using std::size_t;

/**
These functions are concerned with logging latency, load and activity data.

Functions:
 - augment_cumulative()
 - reset_all_loads()
 - reset_all_sync_loads()
 - route_log_basic_latency()
 - route_log_basic_loads()
**/

void augment_cumulative(// supplied data:
                        const size_t       to_add,
                        // supplied/computed data:
                        vector<size_t>&    cumulative)
/*
Hop-length, load, activity and timed activity data is stored cumulatively in vectors, e.g.,
 - [x0, x1, x2, ..., xi, ...] might hold the number xi of healthy routes with hop-length i.
This function adds 'to_add' = i to 'cumulative' = [x0, x1, x2, ...] so that xi becomes xi + 1.
The function works when the vector is empty.
*/
{
    if (cumulative.size() <= to_add)
        cumulative.resize(to_add + 1, 0);
    cumulative[to_add]++;
}

void augment_cumulative(// supplied data:
                        const vector<size_t>&    to_add,
                        // supplied/computed data:
                        vector<size_t>&          cumulative)
/*
Hop-length, load, activity and timed activity data is stored cumulatively in vectors, e.g.,
 - [x0, x1, x2, ..., xi, ...] might hold the number xi of healthy processors with load i
 - [y0, y1, y2, ..., yi, ...] might hold the number yi of healthy servers with load i.
This function adds 'to_add' to 'cumulative' component-wise, e.g., we would get
 - [x0+y0, x1+y1, x2+y2, ..., xi+yi, ...]
and the function works when the vectors have different sizes or are empty.
*/
{
    if (cumulative.size() < to_add.size())
        cumulative.resize(to_add.size(), 0);
    for (size_t i = 0; i < to_add.size(); ++i)
    {
        cumulative[i] += to_add[i];
    }
}

void reset_all_loads(// supplied data:
                     vector<node>&    int_net)
/*
Reset all node/channel loads to 0.
*/
{
    for (size_t nt = 0; nt < System::num_node_types; ++nt)
    {
        for (size_t source = G.node_types[nt].first; source < G.node_types[nt].last; ++source)
        {
            size_t num_ports = int_net[source].node_get_num_ports();
            int_net[source].node_set_load();
            for (size_t port = 0; port < num_ports; ++port)
            {
                int_net[source].node_set_port_load(port);
            }
        }
    }
}

void reset_all_sync_loads(// supplied data:
                          vector<node>&    int_net)
/*
Reset all node/channel timed activity loads to the empty vector. Only ever called if 'ENV.sync' = true.
*/
{
    for (size_t nt = 0; nt < System::num_node_types; ++nt)
    {
        for (size_t source = G.node_types[nt].first; source < G.node_types[nt].last; ++source)
        {
            size_t num_ports = int_net[source].node_get_num_ports();
            int_net[source].node_set_act_load();
            for (size_t port = 0; port < num_ports; ++port)
            {
                int_net[source].node_set_port_act_load(port);
            }
        }
    }
}

void route_log_basic_latency(// supplied data:
                             const RouteStruct&     route,
                             // supplied/computed data:
                             size_t&                failed_routes,
                             size_t&                successful_routes,
                             vector<size_t>&        cumulative_hop_lengths,
                             size_t&                times_tried)
/*
Log whether a route is successful or failed, and if successful then add to the list of cumulative
hop-lengths for the traffic pattern. Also, log the time of attempts made to compute a healthy route.

Functions:
 - augment_cumulative           from here
*/
{
    size_t length = route.port_route.size();
    if (route.error_flag.code == Errors::Good)
    {
        ++successful_routes;
        augment_cumulative(length, cumulative_hop_lengths);
    }
    else
        ++failed_routes;
    times_tried += route.times_tried;
}

void route_log_basic_loads(// supplied data:
                           const struct RouteStruct&     route,
                           //supplied/computed data:
                           vector<node>&                 int_net)
/*
Log a given route in the load variables corresponding to the current traffic pattern. Every node updates
the loads of itself and its own o-ports. There are no loads on i-ports. It is assumed that a route has
been checked for errors which are detailed via the route's error flag so that no load data from bad
routes is ever logged.

In non-synchronous mode, this means just incrementing a load counter by 1. However, things are more
complicated in synchronous mode as we need to keep a count of *when* a node/port begins to actively
manage a message:
 - if a node is the tth node on the route, where t > 0 and t < route-length (i.e., not the source or
   destination) then
    - map[t] = map[t] + 1, for a specific map.
 - if a port is the tth port on the route, where t >= 0 and t < route-length then
    - map[t] = map[t] + 1, for a specific map.
 - if a node/port begins to actively manage a message at time t then this node remains active with this
   message at
    - times t, t + 1, t + 2, ..., t + ('ENV.sync_worm' - 1).
The numbers of activities of a node/port are maintained here.
*/
{
    size_t hop_length = route.port_route.size();// length of a route is <vector of ports>.size()
    if (route.error_flag.code != Errors::Good)
        return;                                 // if failed route then do not log loads
    size_t current_node = route.source;
    for (size_t i = 0; i < hop_length; ++i)
    {
        int_net[current_node].node_inc_port_load(route.port_route[i]);// increment o-port load
        current_node = int_net[current_node].node_get_port_node(System::out, route.port_route[i]);
        if (i != hop_length - 1)                                                          // next node
            int_net[current_node].node_inc_load();// inc. current node load if not the dest.
    }
    if (ENV.sync)                               // log the synchronous load activity variables
    {
        current_node = route.source;            // reset the route source
        for (size_t i = 0; i < hop_length; ++i)
        {
            int_net[current_node].node_inc_port_act_load(route.port_route[i], i);
                                                                // increment o-port activity  at time i
            for (size_t t = 1; t < ENV.sync_worm; ++t)// do for time i+1,...,i+('ENV.sync_worm'-1) too
            {
                int_net[current_node].node_inc_port_act_load(route.port_route[i], i + t);
            }
            current_node = int_net[current_node].node_get_port_node(System::out, route.port_route[i]);
            if (i != hop_length - 1)                                                       // next node
            {
                int_net[current_node].node_inc_act_load(i + 1);// inc. current act load at time
                                                                       //  i + 1 if not the destination
                for (size_t t = 1; t < ENV.sync_worm; t++)// do for time (i+1)+1,(i+1)+2,...,(i+1)+_
                    int_net[current_node].node_inc_act_load(i + 1 + t);//  ('ENV._sync_worm'-1) too
            }
        }
    }
}
