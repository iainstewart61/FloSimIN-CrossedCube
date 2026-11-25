
#ifndef STRUCTS_PACKET_H
#define STRUCTS_PACKET_H

#include <cstddef>

/**
**************
Packet structs
**************
**/

struct Packet
/*
A struct to describe all parameters associated with packets. A global variable 'PKT' will be defined via:
 - Packet PKT;
Any default values are held in 'config_tags'.
*/
{
    std::size_t    packet_size;              // size of a packet (in bits)
    std::size_t    header_size;              // size of a packet header (in bits)
};

#endif
