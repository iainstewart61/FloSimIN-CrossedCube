
#include "node.h"
#include "structs/structs_system.h"
#include "globals.h"

using std::vector;
using std::map;
using std::pair;

/**
This cpp contains the implementations for the class 'node' which is detailed in 'node.h'.
**/

node::node(size_t abs_id, size_t node_type, vector<size_t> node_alg) :
    node_parent_net(nullptr),
    node_type(node_type),
    node_abs_id(abs_id),
    node_alg(std::move(node_alg)),
    node_mode(System::used),
    node_num_ports(),       // needs to be set by user when building later
	node_in_port(),         // needs to be set by user when building later
	node_out_port(),        // needs to be set by user when building later
    node_fault(System::healthy),
	node_load(0),
	node_act_load()
{}

void node::node_set_parent_net(vector<node>* net)
{
    node_parent_net = net;
}

// basic functions

size_t node::node_get_abs_id() const
{
    return node_abs_id;
}

size_t node::node_get_type() const
{
    return node_type;
}

const vector<size_t> &node::node_get_alg() const
{
    return node_alg;
}

int node::node_get_mode() const
{
    return node_mode;
}

size_t node::node_get_num_ports() const
{
    return node_num_ports;
}

bool node::node_get_health() const
{
    return node_fault;
}

int node::node_get_port_mode(size_t in_out, size_t the_port) const
{
    if (in_out == System::in)
        return node_in_port[the_port].port_get_mode();
    else
        return node_out_port[the_port].port_get_mode();
}

bool node::node_get_port_health(size_t in_out, size_t the_port) const
{
    if (in_out == System::in)
        return node_in_port[the_port].port_get_port_health();
    else
        return node_out_port[the_port].port_get_port_health();
}

size_t node::node_get_port_latency(size_t in_out, size_t the_port) const
{
    if (in_out == System::in)
        return node_in_port[the_port].port_get_port_latency();
    else
        return node_out_port[the_port].port_get_port_latency();
}

size_t node::node_get_port_bandwidth(size_t in_out, size_t the_port) const
{
    if (in_out == System::in)
        return node_in_port[the_port].port_get_port_bandwidth();
    else
        return node_out_port[the_port].port_get_port_bandwidth();
}

void node::node_set_alg(vector<size_t> the_node_alg)
{
    node_alg = the_node_alg;
}

void node::node_set_mode(int the_mode)
{
    node_mode = the_mode;
}

void node::node_set_ports(std::size_t the_node_type, std::size_t the_num_ports)
{
    node_num_ports = the_num_ports;
    node_in_port.assign(the_num_ports, port(the_node_type));
    node_out_port.assign(the_num_ports, port(the_node_type));
}

void node::node_set_health(bool the_health)
{
	node_fault = the_health;
}

void node::node_set_port_mode(size_t in_out, size_t the_port, int the_mode)
{
    if (in_out == System::in)
        node_in_port[the_port].port_set_port_mode(the_mode);
    else
        node_out_port[the_port].port_set_port_mode(the_mode);
}

void node::node_set_port_health(size_t in_out, size_t the_port, bool the_health)
{
    if (in_out == System::in)
        node_in_port[the_port].port_set_port_health(the_health);
    else
        node_out_port[the_port].port_set_port_health(the_health);
}

void node::node_set_port_latency(size_t in_out, size_t the_port, size_t the_latency)
{
    if (in_out == System::in)
        node_in_port[the_port].port_set_port_latency(the_latency);
    else
        node_out_port[the_port].port_set_port_latency(the_latency);
}

void node::node_set_port_bandwidth(size_t in_out, size_t the_port, size_t the_bandwidth)
{
    if (in_out == System::in)
        node_in_port[the_port].port_set_port_bandwidth(the_bandwidth);
    else
        node_out_port[the_port].port_set_port_bandwidth(the_bandwidth);
}

// functions in relation to connections

size_t node::node_get_port_node(size_t in_out, size_t the_port) const
{
    if (in_out == System::in)
        return node_in_port[the_port].port_get_port_to_node();
    else
        return node_out_port[the_port].port_get_port_to_node();
}

size_t node::node_get_port_port(size_t in_out, size_t the_port) const
{
    if (in_out == System::in)
        return node_in_port[the_port].port_get_port_to_port();
    else
        return node_out_port[the_port].port_get_port_to_port();
}

void node::node_set_node_and_port(size_t o_port, size_t target_node, size_t target_port)
/*
This is the channel-defining function. Note that target absid/i-port, i.e., a channel, can only be set for
an o-port. When setting for an o-port:
 - the o-port is supplied as 'o_port'
 - the target node of the channel is 'target_node'
 - the i-port at the target node is 'target_port'
 - the o-port 'o_port' at the source is made used (via port_set_port_to_node())
 - the i-port 'target_port' at the target is made used (via port_set_port_to_node())
*/
{
    if (!node_parent_net)
        throw std::runtime_error("node_parent_net is null");
    auto& int_net = *node_parent_net;

    node_out_port[o_port].port_set_port_to_node(target_node);
    node_out_port[o_port].port_set_port_to_port(target_port);
    node& target = int_net[target_node];
    target.node_in_port[target_port].port_set_port_to_node(node_abs_id);
    target.node_in_port[target_port].port_set_port_to_port(o_port);
}

// functions in relation to loads on nodes

size_t node::node_get_load() const
{
    return node_load;
}

const std::map<size_t,size_t>& node::node_get_act_load() const
{
    return node_act_load;
}

void node::node_set_load()
{
    node_load = 0;
}

void node::node_set_act_load()
{
    node_act_load.clear();
}

void node::node_inc_load()
{
    node_load++;
}

void node::node_inc_act_load(size_t the_slot)
{
    node_act_load[the_slot]++;
}

// functions in relation to loads on ports

size_t node::node_get_port_load(size_t the_port) const
{
    return node_out_port[the_port].port_get_port_load_per_pat();
}

const map<size_t,size_t> &node::node_get_port_act_load(size_t the_port) const
{
    return node_out_port[the_port].port_get_port_act_load_per_pat();
}

void node::node_set_port_load(size_t the_port)
{
    node_out_port[the_port].port_set_port_load_per_pat();
}

void node::node_set_port_act_load(size_t the_port)
{
    node_out_port[the_port].port_set_port_act_load_per_pat();
}

void node::node_inc_port_load(size_t the_port)
{
    node_out_port[the_port].port_inc_port_load_per_pat();
}

void node::node_inc_port_act_load(size_t the_port, size_t the_slot)
{
    node_out_port[the_port].port_inc_port_act_load_per_pat(the_slot);
}




