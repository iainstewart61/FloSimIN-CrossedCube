
#ifndef PORT_H
#define PORT_H

#include <map>
#include <cstddef>

class port
/**
The mode of a port is initially set as unused, i.e., 'System::unused', and when a channel is added at that
port, the port becomes used, i.e., 'System::used'. We can also prioritize ports; so 'port_mode' might be
-1, 0, 1, 2, 3, etc. At present, we don't prioritize ports and so every port incident with a channel is
used and remains so throughout with its mode never looked at. Note that whether a port is an output or an
input port is not registered at the port itself: only at the node to which the port is associated.

Port load and activity load information is stored in 'port_load_per_pat' and 'port_act_load_per_pat'. Port
loads are as are node loads.
**/
{
private:
    int port_mode;                          // whether a port is used or unused
	bool port_fault;				        // healthy or faulty
    std::size_t port_to_node;				// absid of the target node to which the port is adjacent
	std::size_t port_to_port;		        // port id of port at target node to which the port is adjacent
	std::size_t port_load_per_pat;			// number of flow-paths in current traffic pattern on which the
                                            //  o-port lies
    std::map<std::size_t,std::size_t> port_act_load_per_pat;// activity number of flow-paths in current tp
                                                            //  on which the port lies (indexed by time t)
	std::size_t port_latency;               // latency of the port
	std::size_t port_bandwidth;             // bandwidth of the port

public:
    port() = delete;                        // forbid default constructor
	port(std::size_t the_node_type);        // constructor

// basic functions
    int port_get_mode() const;                      // get the port's mode (used or unused)
    bool port_get_port_health() const;				// get whether the port is healthy or faulty
    std::size_t port_get_port_latency() const;      // get the port latency
    std::size_t port_get_port_bandwidth() const;    // get the port bandwidth
    void port_set_port_mode(int the_mode);          // set the port's mode
	void port_set_port_health(bool the_health);	    // set the port as healthy or faulty
	void port_set_port_latency(std::size_t the_latency);// set the port latency at 'the_latency'
	void port_set_port_bandwidth(std::size_t the_bandwidth);// set the port bandwidth at 'the_bandwidth'

// functions in relation to connections
	std::size_t port_get_port_to_node() const;	    // get absid of node to which this port is adjacent
	std::size_t port_get_port_to_port() const;		// get port id of port to which this port is adjacent
    void port_set_port_to_node(std::size_t the_node);// set absid of node to which port is adjacent
    void port_set_port_to_port(std::size_t the_port);// set port id of port to which port is adjacent

// functions in relation to loads
	std::size_t port_get_port_load_per_pat() const;	// get number of flow-paths in current tp on which the
                                                    //  port lies
	const std::map<std::size_t,std::size_t> &port_get_port_act_load_per_pat() const;// get activity map of
                                                    //  flow-paths in the current tp on which port lies
	void port_set_port_load_per_pat();              // set number of flow-paths in current tp on which the
                                                    //  port lies at 0
	void port_set_port_act_load_per_pat();          // set activity map of flow-paths in current tp on which
                                                    //  the port lies at empty map
	void port_inc_port_load_per_pat();              // add 1 to number of flow-paths in the current tp on
                                                    //  which the port lies
	void port_inc_port_act_load_per_pat(std::size_t the_slot);// add 1 to activity number of flow-paths in
                                                    //  current tp on which port lies at time 'the_slot'
};

#endif		// PORT_H
