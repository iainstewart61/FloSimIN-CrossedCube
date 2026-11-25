
#include <vector>
#include <string>

#include "data_structures_for_config.h"
#include "../globals.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp file contains functions to generate the two data structures relating to the config namely:
 - 'config_tags'           where all tags and subtags are defined along with any associated global
                           variables and restrictions
 - 'values_and_triples'    where all user-defined values (from enums) for global variables are
                           logged along with their associated tag, subtag and string-equivalent.
These data structures have to be set up before the config file is read. If a new tag or subtag is
introduced then it needs to be added to 'config_tags' here, and if a new user-defined value for
some global variable is introduced it needs to be logged in 'values_and_triples' here.

Functions:
 - create_config_tags()
 - create_values_and_triples()
**/

vector<Tag> create_config_tags()
/*
    Defines all tags and sub-tags for the config corresponding to all the global variables a user can
    define along with all the default values for these global variables. Structure of tag:
     - config tag name
     - whether core or not
     - global group-variable covering the associated global variables
     - vector of 'Subtag' structs
        - subtag name
        - whether core or not
        - whether a new value has been read for the corresponding global variable (added after file read)
        - any restrictions to be imposed when read from config
        - the (reference to the) global variable associated with the subtag name
        - the default value associated with the global variable (sometimes these are dummy and play no role).
        (Note that the default values and the global variables *always* have the same type.)

    The sub-tag name "params" is treated differently to all others. When config is read, we look for, e.g.,
    "params = [n=4, k=5]". So, we need the sub-tag "params" in 'config_tags' and we identify "params" with
    'IN.param_names' (though this association has no impact). When we find "params" in config, we
    immediately process the named list and split it into a list of names and a list of values and store
    these lists in 'IN.param_names' and 'IN.param_values'.

    The config tag names are:
     - <INTERCONNECTIONNETWORK>, <ROUTINGALGORITHM>, <TRAFFICPATTERN>, <FAULT>, <ENVIRONMENT>, <LATENCY>,
       <BANDWIDTH>, <PACKET>, <NOTES>
    */
{
    return
    {
        {
            "<INTERCONNECTIONNETWORK>",
            true,
            false,
            {
                {
                    "name",
                    true,
                    false,
                    Restrictions::NonEmpString,
                    GlobalParamRef{std::ref(IN.name)},
                    GlobalParamVal{string{""}}
                },
                {
                    "params",
                    true,
                    false,
                    Restrictions::VecNonEmpString,
                    GlobalParamRef{std::ref(IN.param_names)},
                    GlobalParamVal{vector<string>{}}
                },
                {
                    "seeds",
                    false,
                    false,
                    Restrictions::Vec8Uint32_t,
                    GlobalParamRef{std::ref(IN.seeds)},
                    GlobalParamVal{vector<uint32_t>{}}
                }
            }
        },
        {
            "<ROUTINGALGORITHM>",
            true,
            false,
            {
                {
                    "name",
                    true,
                    false,
                    Restrictions::NonEmpString,
                    GlobalParamRef{std::ref(RA.name)},
                    GlobalParamVal{string{""}}
                },
                {
                    "dor",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(RA.dor)},
                    GlobalParamVal{RoutingDor::asc}
                },
                {
                    "tiebreak",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(RA.tiebreak)},
                    GlobalParamVal{RoutingTiebreak::inc}
                },
                {
                    "mode",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(RA.mode)},
                    GlobalParamVal{RoutingMode::det}
                },
                {
                    "nondet_repeat",
                    false,
                    false,
                    Restrictions::Gtr0Int,
                    GlobalParamRef{std::ref(RA.nondet_repeat)},
                    GlobalParamVal{std::in_place_type<int>, 1}
                },
                {
                    "increm_bound",
                    false,
                    false,
                    Restrictions::GtrEqMinus1Int,
                    GlobalParamRef{std::ref(RA.increm_bound)},
                    GlobalParamVal{std::in_place_type<int>, -1}
                },
                {
                    "valiant",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(RA.valiant)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "seeds",
                    false,
                    false,
                    Restrictions::Vec8Uint32_t,
                    GlobalParamRef{std::ref(RA.seeds)},
                    GlobalParamVal{vector<uint32_t>{}}
                }
            }
        },
        {
            "<TRAFFICPATTERN>",
            true,
            false,
            {
                {
                    "batches",
                    false,
                    false,
                    Restrictions::NonEmpVecGtr0Size_t,
                    GlobalParamRef{std::ref(TP.batches)},
                    GlobalParamVal{vector<size_t>{}}
                },
                {
                    "seeds",
                    false,
                    false,
                    Restrictions::Vec8Uint32_t,
                    GlobalParamRef{std::ref(TP.seeds)},
                    GlobalParamVal{vector<uint32_t>{}}
                }
            }
        },
        {
            "<FAULT>",
            false,
            false,
            {
                {
                    "fault_check",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(FAU.fault_check)},
                    GlobalParamVal{FaultCheck::none}
                },
                {
                    "input_fault_files",
                    false,
                    false,
                    Restrictions::VecNonEmpFileString,
                    GlobalParamRef{std::ref(FAU.input_fault_files)},
                    GlobalParamVal{vector<string>{}}
                },
                {
                    "output_fault_file",
                    false,
                    false,
                    Restrictions::NonEmpString,
                    GlobalParamRef{std::ref(FAU.output_fault_file)},
                    GlobalParamVal{string{""}}
                },
                {
                    "seeds",
                    false,
                    false,
                    Restrictions::Vec8Uint32_t,
                    GlobalParamRef{std::ref(FAU.seeds)},
                    GlobalParamVal{vector<uint32_t>{}}
                },
                {
                    "limit_guesses",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(FAU.limit_guesses)},
                    GlobalParamVal{std::in_place_type<size_t>, 20000}
                },
                {
                    "show_generation",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(FAU.show_generation)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "sort_faults",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(FAU.sort_faults)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "regenerate",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(FAU.regenerate)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "suffix",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(FAU.suffix)},
                    GlobalParamVal{string{""}}
                }
            }
        },
        {
            "<ENVIRONMENT>",
            false,
            false,
            {
                {
                    "sync",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.sync)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "sync_worm",
                    false,
                    false,
                    Restrictions::Gtr0Size_t,
                    GlobalParamRef{std::ref(ENV.sync_worm)},
                    GlobalParamVal{std::in_place_type<size_t>, 1}
                },
                {
                    "suppress_tps",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.suppress_tps)},
                    GlobalParamVal{std::in_place_type<bool>, true}
                },
                {
                    "use_used_mode",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.use_used_mode)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                    },
                {
                    "view_routes",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.view_routes)},
                    GlobalParamVal{ViewRoutes::none}
                },
                {
                    "view_int_net",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.view_int_net)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "view_int_net_with_faults",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.view_int_net_with_faults)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "view_loads",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.view_loads)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "view_globals",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.view_globals)},
                    GlobalParamVal{ViewGlobals::nondef}
                },
                {
                    "rewrite_registry",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(ENV.rewrite_registry)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                }
            }
        },
        {
            "<LATENCY>",
            false,
            false,
            {
                {
                    "refined_latency",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(LAT.refined_latency)},
                    GlobalParamVal{std::in_place_type<bool>, false}
                },
                {
                    "stack_latency",
                    false,
                    false,
                    Restrictions::Vec4Size_t,
                    GlobalParamRef{std::ref(LAT.stack_latency)},
                    GlobalParamVal{vector<size_t>(System::num_node_types, 0)}
                },
                {
                    "propagation_latency",
                    false,
                    false,
                    Restrictions::Vec4Size_t,
                    GlobalParamRef{std::ref(LAT.propagation_latency)},
                    GlobalParamVal{vector<size_t>(System::num_node_types, 0)}
                },
                {
                    "data_transfer_latency",
                    false,
                    false,
                    Restrictions::Vec4Size_t,
                    GlobalParamRef{std::ref(LAT.data_transfer_latency)},
                    GlobalParamVal{vector<size_t>(System::num_node_types, 0)}
                },
                {
                    "average_routing_latency",
                    false,
                    false,
                    Restrictions::Vec4Size_t,
                    GlobalParamRef{std::ref(LAT.average_routing_latency)},
                    GlobalParamVal{vector<size_t>(System::num_node_types, 0)}
                },
                {
                    "per_hop_latency",
                    false,
                    false,
                    Restrictions::Vec4Size_t,
                    GlobalParamRef{std::ref(LAT.per_hop_latency)},
                    GlobalParamVal{vector<size_t>(System::num_node_types, 0)}
                },
                {
                    "os_recon_latency",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(LAT.os_recon_latency)},
                    GlobalParamVal{std::in_place_type<size_t>, 0}
                }
            }
        },
        {
            "<BANDWIDTH>",
            false,
            false,
            {
                {
                    "port_bandwidth",
                    false,
                    false,
                    Restrictions::Vec16Size_t,
                    GlobalParamRef{std::ref(BAN.port_bandwidth)},
                    GlobalParamVal{vector<size_t>(System::num_channel_types, 0)}
                }
            }
        },
        {
            "<PACKET>",
            false,
            false,
            {
                {
                    "packet_size",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(PKT.packet_size)},
                    GlobalParamVal{std::in_place_type<size_t>, 0}
                },
                {
                    "header_size",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(PKT.header_size)},
                    GlobalParamVal{std::in_place_type<size_t>, 0}
                }
            }
        },
        {
            "<NOTES>",
            false,
            false,
            {
                {
                    "added_note",
                    false,
                    false,
                    Restrictions::None,
                    GlobalParamRef{std::ref(NOT.added_note)},
                    GlobalParamVal{std::in_place_type<std::string>, "-"}
                }
            }
        }
    };
}

vector<ValueAndTriple> create_values_and_triples()
/*
Builds a vector 'values_and_triples' that relates enum values associated with a sub-tag parameter value
to a triple-string consisting of:
 - string for the corresponding tag in config
 - string for the corresponding sub-tag in config
 - the string-equivalent that should appear in config.
It is used when reading data in from config and also when writing output so as to assign a string to an
enum value.
*/
{
    return
    {
        {GlobalParamVal{RoutingDor::asc},                   {"<ROUTINGALGORITHM>","dor","asc"}},
        {GlobalParamVal{RoutingDor::des},                   {"<ROUTINGALGORITHM>","dor","des"}},
        {GlobalParamVal{RoutingDor::ran},                   {"<ROUTINGALGORITHM>","dor","ran"}},

        {GlobalParamVal{RoutingTiebreak::inc},              {"<ROUTINGALGORITHM>","tiebreak","inc"}},
        {GlobalParamVal{RoutingTiebreak::dec},              {"<ROUTINGALGORITHM>","tiebreak","dec"}},
        {GlobalParamVal{RoutingTiebreak::ran},              {"<ROUTINGALGORITHM>","tiebreak","ran"}},

        {GlobalParamVal{RoutingMode::det},                  {"<ROUTINGALGORITHM>","mode","det"}},
        {GlobalParamVal{RoutingMode::nondet},               {"<ROUTINGALGORITHM>","mode","nondet"}},
        {GlobalParamVal{RoutingMode::full},                 {"<ROUTINGALGORITHM>","mode","full"}},
        {GlobalParamVal{RoutingMode::incremental},          {"<ROUTINGALGORITHM>","mode","incremental"}},

        {GlobalParamVal{ViewRoutes::all},                   {"<ENVIRONMENT>","view_routes","all"}},
        {GlobalParamVal{ViewRoutes::bad},                   {"<ENVIRONMENT>","view_routes","bad"}},
        {GlobalParamVal{ViewRoutes::none},                  {"<ENVIRONMENT>","view_routes","none"}},

        {GlobalParamVal{ViewGlobals::none},                 {"<ENVIRONMENT>","view_globals","none"}},
        {GlobalParamVal{ViewGlobals::all},                  {"<ENVIRONMENT>","view_globals","all"}},
        {GlobalParamVal{ViewGlobals::nondef},               {"<ENVIRONMENT>","view_globals","nondef"}},

        {GlobalParamVal{FaultCheck::post},                  {"<FAULT>","fault_check","post"}},
        {GlobalParamVal{FaultCheck::interim_en_route},      {"<FAULT>","fault_check","interim_en_route"}},
        {GlobalParamVal{FaultCheck::interim_en_route_and_post},
                                                    {"<FAULT>","fault_check","interim_en_route_and_post"}},
        {GlobalParamVal{FaultCheck::interim_at_end},        {"<FAULT>","fault_check","interim_at_end"}},
        {GlobalParamVal{FaultCheck::interim_at_end_and_post},
                                                    {"<FAULT>","fault_check","interim_at_end_and_post"}},
        {GlobalParamVal{FaultCheck::none},                  {"<FAULT>","fault_check","none"}},

        {GlobalParamVal{RegistryNetMachine::supercom},      {"<REGISTRY>","net_machine","supercom"}},
        {GlobalParamVal{RegistryNetMachine::dcn},           {"<REGISTRY>","net_machine","dcn"}},

        {GlobalParamVal{RegistryNetType::cartesian},        {"<REGISTRY>","net_type","cartesian"}},
        {GlobalParamVal{RegistryNetType::permutation},      {"<REGISTRY>","net_type","permutation"}},

        {GlobalParamVal{RegistryNetBuild::symmetric},       {"<REGISTRY>","net_build","symmetric"}},
        {GlobalParamVal{RegistryNetBuild::partsymmetric},   {"<REGISTRY>","net_build","partsymmetric"}},
        {GlobalParamVal{RegistryNetBuild::nonsymmetric},    {"<REGISTRY>","net_build","nonsymmetric"}}
    };
}

