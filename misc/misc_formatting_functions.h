
#ifndef MISC_FORMATTING_FUNCTIONS_H
#define MISC_FORMATTING_FUNCTIONS_H

#include <chrono>
#include <string>
#include <vector>
#include <cstddef>
#include <type_traits>
#include <map>

#include "../structs/structs_system.h"

/**
************************************
Functions concerning dates and times
************************************
**/

std::string current_date_time_nice();

std::string current_date_time_suffix();

std::string get_elapsed_time(// supplied data:
                             const std::chrono::high_resolution_clock::time_point&    start,
                             const std::chrono::high_resolution_clock::time_point&    stop);

/**
****************************
Functions concerning spacing
****************************
**/

template <typename T>
std::size_t get_fixed_width(// supplied data:
                            const T&    the_value)
/*
Takes a numeric type value 'the_value' and works out how many digits there are in its string
representation.
*/
{
    static_assert(std::is_arithmetic_v<T> && !std::is_same_v<T, double>,
                  "T must be arithmetic but not double");
    return std::to_string(the_value).size();
}

template <typename T>
std::string truncate_or_pad(// supplied data:
                            const T&    a_value,
                            // supplied/computed data:
                            int         fixed_width)
/*
Takes arithmetic 'a_value', ensures it is >=0, converts it to a string and either truncates it or pads it
with spaces so that it has size |'fixed_width'|:
 - if 'fixed_width' > 0 then padding is at the back/truncating from the back
 - if 'fixed_width' < 0 then padding is at the front/truncating from the front
 - if 'fixed_width' = 0 then nothing is done.
The new string is returned with the original left alone. The type T must be convertible to a string.
*/
{
    static_assert(std::is_arithmetic_v<std::decay_t<T>> || std::is_same_v<std::decay_t<T>, std::string>,
                  "T must be a numeric or string type");

    std::string output_string;                                  // the string to be output
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) // convert to string if needs be
        output_string = a_value;
    else
    {
        if constexpr (std::is_signed_v<std::decay_t<T>>)
        {
            auto value = (a_value < 0) ? -a_value : a_value;
            output_string = std::to_string(value);
        }
        else
            output_string = std::to_string(a_value);
    }
    int string_size = static_cast<int>(output_string.size());   // safe to cast as size never too large

    if (fixed_width > 0)
    {
        if (string_size < fixed_width)                          // if string needs to be padded at back
            output_string.append(fixed_width - string_size, ' ');
        else if (string_size > fixed_width)                     // if string needs to be truncated from back
            output_string = output_string.substr(0, fixed_width);
    }
    else if (fixed_width < 0)
    {
        fixed_width = fixed_width * (-1);
        if (string_size < fixed_width)                          // if string needs to be padded at front
            output_string.insert(0, fixed_width - string_size, ' ');
        else if (string_size > fixed_width)                     // if string needs to be truncated from front
            output_string = output_string.substr(string_size - fixed_width);
    }
    return output_string;
}

/**
******************************************
Functions turning non-strings into strings
******************************************
**/

std::string get_a_percent_string(// supplied data:
                                 const std::size_t    numerator,
                                 const std::size_t    denominator);

/**
*************************************
Functions concerning maps and vectors
*************************************
**/

void make_dense_vector(// supplied data:
                       const std::map<size_t, size_t>&    the_map,
                       // computed data:
                       std::vector<std::size_t>&          map_as_vector);

std::string format_double(// supplied data:
                          const double         value,
                          const std::size_t    decimals);

template <typename T>
std::string vector_to_a_string(// supplied data:
                               const std::vector<T>&    a_vector,
                               const int                spacing)
/*
Convert an arithmetic or string vector into a string of the form:
 - [*,*,...,*]
and return the string. If 'spacing' != 0 then we set each component (as a string) to have size |'spacing'|
by:
 - padding at the back/truncating from the back, if 'spacing' > 0
 - padding at the front/truncating from the front, if 'spacing' < 0
 - doing nothing, if 'spacing' = 0.

Functions:
 - format_double            from here
 - truncate_or_pad          from here
*/
{
    static_assert(std::is_arithmetic_v<std::decay_t<T>> || std::is_same_v<std::decay_t<T>, std::string>,
                  "T must be arithmetic or std::string");

    std::string the_string;
    std::string output_string = "[";
    std::size_t a_vec_size = a_vector.size();
    for (std::size_t i = 0; i < a_vec_size; ++i)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, double>)// if double
            the_string = format_double(a_vector[i], System::dp_precision);
        else if constexpr (std::is_arithmetic_v<std::decay_t<T>>)// if arithmetic but not double
            the_string = std::to_string(a_vector[i]);       // convert to string
        else                                                // if non-arithmetic type (string is only non-arithmetic type)
            the_string = a_vector[i];                       // keep as string
        the_string = truncate_or_pad(the_string, spacing);
        output_string += the_string;
        if (i + 1 < a_vec_size)
            output_string += ",";
    }
    output_string += "]";
    return output_string;
}

#endif
