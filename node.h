
#ifndef NODE_H
#define NODE_H

#include <vector>
#include <map>
#include <cstddef>

#include "port.h"

class node
/**
A channel runs from a node u to a node v. It has an output-port (o-port) o at u and an input port
(i-port) i at v:
 - u -o---------i-> v
In general, there are nodes of types:
 - processor
 - server
 - switch
 - optical switch.
Every node has the same number of i-ports as o-ports and this number is stored as part of a node's
definition as an object of the class 'node' (see 'node_num_ports').

Every node has:
 - an absolute identifier  (absid) in [0, 'G.total_nodes' - 1]            : see 'node_abs_id'
 - a relative identifier   (relid) in, e.g., [0, 'G.num_nodes[G.se]' - 1]
 - an algebraic identifier (algid) which is a vector<size_t>              : see 'node_alg'.
There is a bijection between relid's and algid's of nodes of a given type and these bijections must
be supplied by the user when defining an interconnection network via:
 - '<int_net>_alg_convert_to_id'
 - '<int_net>_alg_convert_from_id'.

When a node is built, its mode 'node_mode' is initially set as used via 'used' = 'System::used'.
Having used and unused nodes has a number of potential uses, e.g.:
 - it allows us to define sub-networks of the usual interconnection networks and we might
   prioritize used nodes, say, if we wish by associating varying integer values
    * this facility is as yet not implemented
 - we can define networks where a node might have a different number of input ports than output
   ports
    * this facility is as yet not implemented.
This is for the future and currently all nodes are set as used when constructed with the mode of
any node never looked at.

When building a node, the parameters to supply are:
 - 'the_abs_id'     (the absolute id)
 - 'the_node_type'  (processor, server, etc.)
 - 'the_node_alg'   (the algebraic id)
The number of ports and the vectors of ports must be built in:
 - '<int_net>_build'
by the user when building the interconnection network.

Each o-port (resp. i-port) at a source node has details of:
 - the target node to which it is adjacent
 - the i-port (resp. o-port) to which it is adjacent at the target node.
It will usually be the case that an interconnection network construction is symmetric so that
 - for any channel x -> y from o-port o_x to i-port i_y
    - there is a channel y -> x from o-port o_y to i-port i_x
    - o_x = i_x = o_y = i_y
An interconnection network is part-symmetric if o_x = i_x and o_y = i_y.

Note that this class 'node' needs access to 'int_net' but I don't want to have to supply this
parameter every time. Every node has a pointer 'node_parent_net' to 'int_net' and a setter for this
pointer, namely node_set_parent_net() (see below). The function:
 - prelude_to_int_net_build()
from 'int_net_construction' needs to be called immediately after the interconnection network has been
defined and before the interconnection network has been built in order to set this up.

As regards synchronous loads, when we compute a route, the time index for nodes is as follows:
 - the source appears on the route at time 0
 - the next node at time 1; ...; and
 - the destination at time t, where t is the length of the route.
The time index for ports is:
 - the output port from the source appears on the route at time 0
 - the next output port at time 1; ...; and
 - the last output port at time t - 1, where t is the length of the route.
We call an occurrence of some node/port on some route at some time t an activity (as the node/port
is active at time t). The activity load is a map for some node or port that is indexed by the time
t and denotes the number of activities at time t. We only keep load and activity load data for
output ports (so all load and activity load data for input ports is 0). The reasoning is that if
we ever use port load or activity load data then we will be working with an adaptive routing
algorithm and we would only ever need the loads or activity loads of output ports to advise on the
next channel to take.

As regards faults, we only set output ports as faulty (so all input ports are healthy even if the
underlying channel is deemed to be faulty). This is because channel fault information is only used
for route building which is all about choosing an output port to take.
**/
{
private:
    std::vector<node>* node_parent_net;                 // a pointer to 'int_net'
    std::size_t node_type;                              // the type of the node
    std::size_t node_abs_id;                            // the absolute id of the node in 'int_net'
    std::vector<std::size_t> node_alg;                  // algebraic identifier of node
    int node_mode;                                      // currently always set as used
    std::size_t node_num_ports;                         // the number of (input or output) ports
	std::vector<port> node_in_port;                     // vector of input ports
	std::vector<port> node_out_port;                    // vector of output ports
	bool node_fault;								    // whether node is healthy or faulty
	std::size_t node_load;					            // the number of flow-paths in the current traffic
                                                        //  pattern on which the node (internally) lies
    std::map<std::size_t,std::size_t> node_act_load;    // number of flow-paths in current traffic
                                                        //  pattern on which the node (internally)
public:                                                 //  lies and with the map indexed by time t
    node() = delete;                                    // forbid default constructor
	node(const std::size_t abs_id,
         const std::size_t node_type,
         const std::vector<std::size_t> node_alg);

    void node_set_parent_net(std::vector<node> *net);   // sets 'int_net' for a node

    // basic functions

    std::size_t node_get_abs_id() const;                // get the node's abs_id
    std::size_t node_get_type() const;                  // get the node's type
    const std::vector<std::size_t> &node_get_alg() const;// get the node's algid

    int node_get_mode() const;                          // get node's mode (always used)
    std::size_t node_get_num_ports() const;             // get the number of (input or output) ports
    bool node_get_health() const;						// get whether the node is healthy or faulty
    int node_get_port_mode(size_t in_out, std::size_t the_port) const;// get a port's mode (always used)
    bool node_get_port_health(size_t in_out, std::size_t the_port) const;// get a port's health
    std::size_t node_get_port_latency(size_t in_out, std::size_t the_port) const;// get port latency
    std::size_t node_get_port_bandwidth(size_t in_out, std::size_t the_port) const;// get port bandwidth

    void node_set_alg(std::vector<std::size_t> the_node_alg);// set the algid
    void node_set_mode(int the_mode);                   // set a node's mode
    void node_set_ports(std::size_t the_node_type, std::size_t the_num_ports);// build all ports
    void node_set_health(bool the_health);				// set a node's health
    void node_set_port_mode(size_t in_out, std::size_t the_port, int the_mode);// set a port's mode
    void node_set_port_health(size_t in_out, std::size_t the_port, bool the_health);// set a port's health
	void node_set_port_latency(size_t in_out, std::size_t the_port, std::size_t the_latency);
                                                                                    // set port latency
	void node_set_port_bandwidth(size_t in_out, std::size_t the_port, std::size_t the_bandwidth);
                                                                                    // set port bandwidth
    // functions in relation to connections

    std::size_t node_get_port_node(size_t in_out, std::size_t the_port) const;// get absid of node to which a
                                                                           //  port is adjacent
    std::size_t node_get_port_port(size_t in_out, std::size_t the_port) const;// get port id of port to which
                                                                           //  the port is adjacent
    void node_set_node_and_port(std::size_t o_port, std::size_t target_node, std::size_t target_port);
                                                        // set absid and i-port at target for channel from
    // functions in relation to loads on nodes              //  some o-port at source: channel-defining function

	std::size_t node_get_load() const;			        // get number of flow-paths in the current tp on
                                                        //  which the node lies
    const std::map<std::size_t,std::size_t> &node_get_act_load() const;// get activity map holding
                                                        //  number of flow-paths in current tp on which node
                                                        //  lies, indexed by time t
    void node_set_load();                               // set number of flow-paths in current tp on which
                                                        //  the node lies to be 0
    void node_set_act_load();                           // set activity map of flow-paths in the current tp
                                                        //  on which the node lies to be the empty map
    void node_inc_load();                               // add 1 to number of flow-paths in the current tp
                                                        //  on which the node lies
    void node_inc_act_load(std::size_t the_slot);       // add 1 to activity number of flow-paths in the
                                                        //  current tp on which the node lies at time
    // functions in relation to load on ports           //  t = 'the_slot'

    std::size_t node_get_port_load(std::size_t o_port) const;// get number of flow-paths in current
                                                        //  tp on which o-port 'o_port' lies: note that loads
                                                        //  are only registered at o-ports
    const std::map<std::size_t,std::size_t>& node_get_port_act_load(std::size_t o_port) const;
                                                        // get activity map holding number of flow-paths in
                                                        //  current tp on which o-port 'o_port' lies
    void node_set_port_load(std::size_t o_port);        // set number of flow-paths in current tp on which
                                                        //  o-port 'o_port' lies at 0
    void node_set_port_act_load(std::size_t o_port);    // set activity number of flow-paths in current
                                                        //  tp on which o-port 'o_port' lies at empty map
    void node_inc_port_load(std::size_t o_port);        // add 1 to number of flow-paths in current tp on
                                                        //  which o-port 'o_port' lies
    void node_inc_port_act_load(std::size_t o_port, std::size_t the_slot);// add 1 to activity number
                                                        //  of flow-paths in current tp on which o-port
                                                        //  'o_port' lies by inc. map entry at key 'the_slot'
};

#endif		// NODE_H

