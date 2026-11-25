
#ifndef MISC_STRING_MANIPULATION_H
#define MISC_STRING_MANIPULATION_H

#include <string>
#include <cstddef>
#include <vector>
#include <limits>
#include <type_traits>
#include <cstdint>
#include <stdexcept>

#include "../errors/error_writing.h"

std::string strip_string(// supplied/computed data:
                         const std::string&    the_string);

Errors look_for_delimiters(// supplied data:
                           const std::string&    the_string,
                           const std::string&    left_bookend,
                           const std::string&    right_bookend,
                           // computed data:
                           std::size_t&          pos_left,
                           std::size_t&          pos_right);

std::vector<std::string> split_string(// supplied data:
                                      const std::string&    the_string,
                                      const std::string&    delimiter);

template<typename T>
Errors string_to_value(// supplied data:
                       const std::string&    suffix,
                       // computed data:
                       T&                    value)
/*
Converts 'suffix' to 'value' with:
 - T being size_t, double or int.
We check that 'suffix' does not start with '-' for size_t as this leads to wrap-around difficulties. Note
that for 'suffix' to be valid, all of the characters must contribute to the size_t, double or int.

Errors:
 - Good, NotValidInt, NotValidSize_t, NotValidDouble, OutOfRangeInt, OutOfRangeSize_t, OutOfRangeDouble
*/
{
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, size_t> || std::is_same_v<T, double>,
                  "T must be int, size_t or double");

    try
    {
        size_t pos = 0;

        if constexpr (std::is_same_v<T, int>)
        {
            int tmp = std::stoi(suffix, &pos);
            if (pos != suffix.size())
                return Errors::NotValidInt;
            value = tmp;
        }
        else if constexpr (std::is_same_v<T, size_t>)
        {
            if ((!suffix.empty()) && (suffix[0] == '-'))
                return Errors::NotValidSize_t;
            auto tmp = std::stoull(suffix, &pos);
            if (tmp > std::numeric_limits<size_t>::max())
                return Errors::OutOfRangeSize_t;
            if (pos != suffix.size())
                return Errors::NotValidSize_t;
            value = static_cast<size_t>(tmp);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            double tmp = std::stod(suffix, &pos);
            if (pos != suffix.size())
                return Errors::NotValidDouble;
            value = tmp;
        }
    }
    catch (const std::invalid_argument&)
    {
        if constexpr (std::is_same_v<T, int>)
            return Errors::NotValidInt;
        if constexpr (std::is_same_v<T, size_t>)
            return Errors::NotValidSize_t;
        if constexpr (std::is_same_v<T, double>)
            return Errors::NotValidDouble;
    }
    catch (const std::out_of_range&)
    {
        if constexpr (std::is_same_v<T, int>)
            return Errors::OutOfRangeInt;
        if constexpr (std::is_same_v<T, size_t>)
            return Errors::OutOfRangeSize_t;
        if constexpr (std::is_same_v<T, double>)
            return Errors::OutOfRangeDouble;
    }
    return Errors::Good;
}

void get_a_raw_list_of_string_from_a_suffix(// supplied data:
                                            const std::string&           suffix,
                                            const std::string&           left_bookend,
                                            const std::string&           right_bookend,
                                            // computed data:
                                            std::vector<std::string>&    list_of_string);

template <typename T>
Errors build_list_of_num_from_strings(// supplied data:
                                      const std::vector<std::string>&    list_of_strings,
                                      // computed data:
                                      std::vector<T>&                    list_of_num)
/*
Builds a list of T from a list of strings with:
 - T being size_t or uint32_t.
If there is an error then it is signalled and the empty list is returned. Note that it is possible that
'list_of_strings' is empty when Good will be returned along with an empty 'list_of_num'.

Errors:
 - Good, NotValidListOfUint32_t, NotValidListOfSize_t
*/
{
    static_assert(std::is_same_v<T, std::size_t> || std::is_same_v<T, std::uint32_t>,
                  "T must be size_t or uint32_t");

    list_of_num.clear();
    list_of_num.reserve(list_of_strings.size());

    try
    {
        for (const auto& an_item : list_of_strings)
        {
            size_t pos = 0;
            auto v = std::stoull(an_item, &pos);

            if (pos != an_item.size())
            {
                list_of_num.clear();
                if constexpr (std::is_same_v<T, std::uint32_t>)
                    return Errors::NotValidListOfUint32_t;
                else if constexpr (std::is_same_v<T, std::size_t>)
                    return Errors::NotValidListOfSize_t;
            }

            if constexpr (std::is_same_v<T, std::uint32_t>)
            {
                if (v > std::numeric_limits<std::uint32_t>::max())
                {
                    list_of_num.clear();
                    return Errors::NotValidListOfUint32_t;
                }
                list_of_num.push_back(static_cast<std::uint32_t>(v));
            }
            else if constexpr (std::is_same_v<T, std::size_t>)
            {
                if (v > std::numeric_limits<std::size_t>::max())
                {
                    list_of_num.clear();
                    return Errors::NotValidListOfSize_t;
                }
                list_of_num.push_back(static_cast<std::size_t>(v));
            }
        }
    }
    catch (const std::invalid_argument&)    // errors from std::stoull
    {
        list_of_num.clear();
        if constexpr (std::is_same_v<T, std::uint32_t>)
            return Errors::NotValidListOfUint32_t;
        else if constexpr (std::is_same_v<T, std::size_t>)
            return Errors::NotValidListOfSize_t;
    }
    catch (const std::out_of_range&)
    {
        list_of_num.clear();
        if constexpr (std::is_same_v<T, std::uint32_t>)
            return Errors::NotValidListOfUint32_t;
        else if constexpr (std::is_same_v<T, std::size_t>)
            return Errors::NotValidListOfSize_t;
    }
    return Errors::Good;
}

void get_spacey_suffix(// supplied data:
                       const std::string&    line_string,
                       const std::string&    prefix,
                       //computed data:
                       std::string&          suffix,
                       std::string&          stripped_string);

#endif
