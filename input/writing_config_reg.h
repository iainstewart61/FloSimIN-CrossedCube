
#ifndef WRITING_CONFIG_REG_H
#define WRITING_CONFIG_REG_H

#include <string>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <ostream>
#include <cstdint>
#include <cstddef>

#include "data_structures_for_config.h"
#include "read_registry_file.h"
#include "../globals.h"
#include "../structs/structs_system.h"
#include "../structs/structs_routing.h"
#include "../structs/structs_environment.h"
#include "../errors/error_writing.h"
#include "../misc/misc_formatting_functions.h"

/**
*********************
Writing a single line
*********************
**/

template <typename T>
void write_line(//supplied data:
                const std::string&                    left_string,
                int                                   left_pad,
                const T&                              right_structure,
                const std::vector<ValueAndTriple>&    values_and_triples,
                // other stuff:
                std::ostream&                         output)
/*
Writes:
 - 'truncate_or_pad(left_string, left_pad) + ': ' + 'right_string' + '\n'
where
 - 'right_string' = 'right_structure' if the type is not user-defined
 - 'right_string' = appropriate string-value if the type is user-defined.

Functions:
 - truncate_or_pad              from misc_formatting_functions
 - coding                       from error_writing
 - vector_to_a_string           from misc_formatting_functions
*/
{
    output << truncate_or_pad(left_string, left_pad) <<": ";

    // user-defined structs

    if constexpr(std::is_same_v<std::decay_t<T>, RoutingDor>
                 || std::is_same_v<std::decay_t<T>, RoutingTiebreak>
                 || std::is_same_v<std::decay_t<T>, RoutingMode>
                 || std::is_same_v<std::decay_t<T>, ViewRoutes>
                 || std::is_same_v<std::decay_t<T>, ViewGlobals>
                 || std::is_same_v<std::decay_t<T>, FaultCheck>
                 || std::is_same_v<std::decay_t<T>, RegistryNetMachine>
                 || std::is_same_v<std::decay_t<T>, RegistryNetType>
                 || std::is_same_v<std::decay_t<T>, RegistryNetBuild>)
    {
        GlobalParamVal target = right_structure;
        auto it = std::find_if(values_and_triples.begin(), values_and_triples.end(),
                            [&](const ValueAndTriple& item)
                            {
                                return item.value == target;
                            });

        if (it != values_and_triples.end())
            output << it->triple.value << "\n";
        else
            coding(Errors::TypeMismatch);
    }

    // non-user-defined structs

    else if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
        output << right_structure << "\n";
    else if constexpr(std::is_same_v<std::decay_t<T>, std::vector<std::string>>
                      || std::is_same_v<std::decay_t<T>, std::vector<std::size_t>>
                      || std::is_same_v<std::decay_t<T>, std::vector<std::uint32_t>>)
        output << vector_to_a_string(right_structure, 0) << "\n";
    else if constexpr(std::is_same_v<std::decay_t<T>, int>
                      || std::is_same_v<std::decay_t<T>, std::size_t>)
        output << std::to_string(right_structure) << "\n";
    else if constexpr(std::is_same_v<std::decay_t<T>, bool>)
        output << (right_structure ? "true\n" : "false\n");
    else
        coding(Errors::TypeMismatch);
}

bool is_default(// supplied data:
                const std::vector<Tag>&    config_tags,
                const std::string&         main_tag,
                const std::string&         var);

template <typename T>
void write_line_wrt_default(//supplied data:
                            const std::vector<Tag>&               config_tags,
                            const std::string&                    main_tag,
                            const std::string&                    var,
                            std::string                           left_string,
                            int                                   left_pad,
                            const T&                              right_structure,
                            const std::vector<ValueAndTriple>&    values_and_triples,
                            // other stuff:
                            std::ostream&                         output)
/*
If ENV.view_globals == ViewGlobals::all and the value is non-default then write:
 - 'truncate_or_pad(left_string + '*', left_pad) + ': ' + 'right_string' + '\n'
if ENV.view_globals == ViewGlobals::all and the value is default then writes:
 - 'truncate_or_pad(left_string, left_pad) + ': ' + 'right_string' + '\n'
if ENV.view_globals == ViewGlobals::nondef and the value is non-default then writes:
 - 'truncate_or_pad(left_string, left_pad) + ': ' + 'right_string' + '\n'
if ENV.view_globals == ViewGlobals::nondef and the value is default then writes nothing
where:
 - 'right_string' = 'right_structure' if the type is not user-defined
 - 'right_string' = appropriate string-value if the type is user-defined.

Functions:
 - is_default               from here
 - write_line               from here
*/
{
    bool was_it_read = is_default(config_tags, main_tag, var);
    if (ENV.view_globals == ViewGlobals::all)
    {
        if (was_it_read)
            left_string += "*";
    }
    else if (ENV.view_globals == ViewGlobals::nondef)
    {
        if (!was_it_read)
            return;
    }
    write_line(left_string, left_pad, right_structure, values_and_triples, output);
}

/**
************************
Writing traffic patterns
************************
**/

/**
**************************
Writing fault descriptions
**************************
**/

/**
************************************
Writing the contents of the registry
************************************
**/

/**
***************
Writing globals
***************
**/

void write_globals(// supplied stuff:
                   const std::vector<Tag>&               config_tags,
                   const int                             left_pad,
                   const TheRegistry&                    the_registry,
                   const std::vector<ValueAndTriple>&    values_and_triples,
                   // other stuff:
                   std::ostream&                         output);

#endif
