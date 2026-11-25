
#ifndef READ_CONFIG_FILE_H
#define READ_CONFIG_FILE_H

#include <string>
#include <cstddef>
#include <vector>
#include <type_traits>
#include <cstdint>

#include "data_structures_for_config.h"
#include "../misc/misc_string_functions.h"
#include "../errors/error_writing.h"
#include "../structs/structs_system.h"

/**
*******************************************************
Functions involving the assignment of values to globals
*******************************************************
**/

void assign_to_global(// supplied data:
                      GlobalParamVal&          value,
                      GlobalParamRef&          var);

/**
****************************
Functions for parsing config
****************************
**/

/**
**************************************************
Functions for filtering values w.r.t. restrictions
**************************************************
**/

Errors filter_an_int(//supplied data:
                     const int    an_int,
                     Subtag*      prefix_ptr);

template <typename T>
Errors filter_a_list_of_num(//supplied data:
                            const std::vector<T>&    list_of_num,
                            const Subtag*            prefix_ptr)
/*
Checks any restriction on 'list_of_num' w.r.t. 'prefix_ptr->restriction'. Note that:
 - T must be size_t or uint32_t.

Errors:
 - Good, EmptyListOfSize_t, NotValidListOfSize_t, BadLengthList
*/
{
    static_assert(std::is_same_v<T, std::size_t> || std::is_same_v<T, std::uint32_t>,
                  "T must be size_t or uint32_t");

    if (prefix_ptr->restriction == Restrictions::NonEmpVecGtr0Size_t)
    {
        if (list_of_num.empty())
            return Errors::EmptyListOfSize_t;
        else
        {
            for (const auto& item : list_of_num)
            {
                if (item == 0)
                    return Errors::NotValidListOfSize_t;
            }
        }
    }
    else if (prefix_ptr->restriction == Restrictions::Vec8Uint32_t)
    {
        if (list_of_num.size() != System::num_seeds)
            return Errors::BadLengthList;
    }
    else if (prefix_ptr->restriction == Restrictions::Vec4Size_t)
    {
        if (list_of_num.size() != System::num_node_types)
            return Errors::BadLengthList;
    }
    else if (prefix_ptr->restriction == Restrictions::Vec16Size_t)
    {
        if (list_of_num.size() != System::num_channel_types)
            return Errors::BadLengthList;
    }
    return Errors::Good;
}

/**
********************************************************
Functions for getting standard-type values from a suffix
********************************************************
**/

template <typename T>
Errors get_a_num_from_a_suffix(// supplied data:
                               const std::string&    suffix,
                               // supplied/computed data:
                               Subtag*               prefix_ptr)
/*
Try to resurrect a T from 'suffix' where:
 - T is int or size_t.
After resurrecting a T, we filter for restrictions before setting the global variable corresponding to the
subtag to this T, otherwise we leave it alone (as it has been assigned its default value) and just flag the
error. Note that if we do set a global variable value then this is registered in 'config_tags'. We require
that 'prefix_ptr->global_var' has type T and 'assign_to_global' will throw a coding error if it doesn't and
terminate.

Errors:
 - Good, NotValidInt, NotValidSize_t, OutOfRangeInt, OutOfRangeSize_t

Functions:
 - string_to_value          from misc_string_functions
 - filter_an_int            from here
 - assign_to_global         from here
*/
{
    static_assert(std::is_same_v<T, std::size_t> || std::is_same_v<T, int>,
                  "T must be size_t or int");

    T a_value{};
    Errors error = string_to_value(suffix, a_value);      // check whether we can resurrect a T
    if (error == Errors::Good)
    {
        if constexpr (std::is_same_v<T, int>)
            error = filter_an_int(a_value, prefix_ptr);   // check restrictions on the int
        else if constexpr (std::is_same_v<T, std::size_t>)
            error = filter_a_size_t(a_value, prefix_ptr); // check restrictions on the size_t
        if (error == Errors::Good)
        {
            GlobalParamVal GPV_a_value = a_value;
            assign_to_global(GPV_a_value, prefix_ptr->global_var);
            prefix_ptr->value_read = true;
        }
    }
    return error;
}

template <typename T>
Errors get_a_list_of_num_from_a_suffix(// supplied data:
                                       std::string&    suffix,
                                       // supplied/computed data:
                                       Subtag*         prefix_ptr)
/*
Try to resurrect a list of T from 'suffix' where:
 - T is size_t or uint32_t.
After resurrecting a list, we filter for restrictions before setting the global variable corresponding to the
subtag to this list, otherwise we leave it alone (as it has been assigned its default value) and just flag
the error. Note that if we do set a global variable value then this is registered in 'config_tags'. We
require that 'prefix_ptr->global_var' has type vector<T> and 'assign_to_global' will throw a coding error
if it doesn't and terminate.

Errors:
 - Good, NotValidListOfUint32_t, NotValidListOfSize_t, EmptyListOfSize_t, BadLengthList

Functions:
 - get_a_raw_list_of_string_from_a_suffix           from misc_string_functions
 - build_list_of_num_from_strings                   from misc_string_functions
 - filter_a_list_of_num                             from here
 - assign_to_global                                 from here
*/
{
    Errors error;
    std::vector<T> list_of_num;
    std::vector<std::string> list_of_strings;
    get_a_raw_list_of_string_from_a_suffix(suffix, "[", "]", list_of_strings);
    error = build_list_of_num_from_strings(list_of_strings, list_of_num);
    if (error != Errors::Good)
        return error;
    error = filter_a_list_of_num(list_of_num, prefix_ptr);
    if (error != Errors::Good)
        return error;
    GlobalParamVal GPV_list_of_num = list_of_num;
    assign_to_global(GPV_list_of_num, prefix_ptr->global_var);
    prefix_ptr->value_read = true;
    return Errors::Good;
}

/**
************************************************************
Functions for getting user-defined-type values from a suffix
************************************************************
**/

/**
************************************************
Functions for getting a named-list from a suffix
************************************************
**/

/**
************************************
Register all values read from config
************************************
**/

/**
************************************************
Deal with potential traffic pattern descriptions
************************************************
**/

/**
**************************************
Deal with potential fault descriptions
**************************************
**/

/**
***************
Read the config
***************
**/

void read_config_file(// supplied data:
                      const std::string&              input_file_name,
                      // computed data:
                      std::vector<Tag>&               config_tags,
                      std::vector<ValueAndTriple>&    values_and_triples,
                      // other stuff:
                      std::ostream&                   output);

#endif

