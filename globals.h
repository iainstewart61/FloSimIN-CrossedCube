
#ifndef GLOBALS_H
#define GLOBALS_H

#include "structs/structs_bandwidth.h"
#include "structs/structs_environment.h"
#include "structs/structs_fault.h"
#include "structs/structs_int_net.h"
#include "structs/structs_latency.h"
#include "structs/structs_map.h"
#include "structs/structs_note.h"
#include "structs/structs_packet.h"
#include "structs/structs_registry.h"
#include "structs/structs_routing.h"
#include "structs/structs_system.h"
#include "structs/structs_traffic_pattern.h"
#include "random_gen.h"

extern System G;

void initialize_system_globals();

extern InterconnectionNetwork IN;
extern RoutingAlgorithm RA;
extern TrafficPattern TP;
extern Environment ENV;
extern Fault FAU;
extern Latency LAT;

void latency_updating();

extern Bandwidth BAN;
extern Packet PKT;
extern Notes NOT;
extern RegistryGlobals RG;
extern RoutingAlgorithmPrelude RAP;

extern RandomGenerator RNG_fault;
extern RandomGenerator RNG_tp;
extern RandomGenerator RNG_misc;
extern RandomGenerator RNG_routing;

void globals_housekeeping();

extern Maps M;

#endif
