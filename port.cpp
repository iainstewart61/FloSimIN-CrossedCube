
#include "port.h"
#include "structs/structs_system.h"
#include "globals.h"

using std::map;
using std::pair;
using std::size_t;

/**
This cpp contains the implementations for the class 'port' which is detailed in 'port.h'.
**/

port::port(size_t node_type) :
    port_mode(System::unused),
    port_fault(System::healthy),
	port_to_node(System::unused),
	port_to_port(System::unused),
	port_load_per_pat{0},
	port_act_load_per_pat(),
	port_latency(LAT.per_hop_latency[node_type]),
    port_bandwidth(BAN.port_bandwidth[node_type])
{}

// basic functions

int port::port_get_mode() const
{
    return port_mode;
}

bool port::port_get_port_health() const
{
    return port_fault;
}

size_t port::port_get_port_latency() const
{
    return port_latency;
}

size_t port::port_get_port_bandwidth() const
{
    return port_bandwidth;
}

void port::port_set_port_mode(int the_mode)
{
    port_mode = the_mode;
}

void port::port_set_port_health(bool the_health)
{
	port_fault = the_health;
}

void port::port_set_port_latency(size_t the_latency)
{
    port_latency = the_latency;
}

void port::port_set_port_bandwidth(size_t the_bandwidth)
{
    port_bandwidth = the_bandwidth;
}

// functions in relation to connections

size_t port::port_get_port_to_node() const
{
	return port_to_node;
}

size_t port::port_get_port_to_port() const
{
	return port_to_port;
}

void port::port_set_port_to_node(size_t the_node)
{
    port_to_node = the_node;
    port_mode = System::used;
}

void port::port_set_port_to_port(size_t the_port)
{
    port_to_port = the_port;
}

// functions in relation to loads

size_t port::port_get_port_load_per_pat() const
{
    return port_load_per_pat;
}

const map<size_t,size_t> &port::port_get_port_act_load_per_pat() const
{
    return port_act_load_per_pat;
}

void port::port_set_port_load_per_pat()
{
    port_load_per_pat = 0;
}

void port::port_set_port_act_load_per_pat()
{
    port_act_load_per_pat.clear();
}

void port::port_inc_port_load_per_pat()
{
    port_load_per_pat++;
}

void port::port_inc_port_act_load_per_pat(size_t the_slot)
{
    port_act_load_per_pat[the_slot]++;
}

