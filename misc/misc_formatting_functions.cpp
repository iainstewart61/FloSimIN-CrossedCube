
#include "misc_formatting_functions.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp contains miscellaneous functions concerning formatting data.

Functions concerning dates and times:
 - current_date_time_nice()
 - current_date_time_suffix()
 - get_elapsed_time()
Functions concerning spacing:
 - get_fixed_width()
 - truncate_or_pad()
Functions turning non-strings into strings:
 - get_a_percent_string()
Functions concerning maps and vectors:
 - make_dense_vector()
 - format_double()
 - vector_to_a_string()
**/

/**
************************************
Functions concerning dates and times:
************************************
**/

string current_date_time_nice()
/*
Returns the current date and time in a nicely formatted string, e.g.
 - 06:55am 5 Nov 2025
*/
{
    using clock = std::chrono::system_clock;

    auto now = clock::now();
    std::time_t tt = clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%I:%M%p %d %b %Y");
    string result = oss.str();
    auto pos = result.find_first_of("AP");
    if (pos != std::string::npos)
    {
        result[pos]     = std::tolower(result[pos]);
        result[pos + 1] = std::tolower(result[pos + 1]);
    }
    return result;
}

string current_date_time_suffix()
/*
Returns a suffix to be used to time-stamp an output file. The suffix is of the form:
 - 2024-11-08-170537
*/
{
    using clock = std::chrono::system_clock;

    auto now = clock::now();
    std::time_t tt = clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d-%H%M%S");
    return oss.str();
}

string get_elapsed_time(// supplied data:
                        const std::chrono::high_resolution_clock::time_point&    start,
                        const std::chrono::high_resolution_clock::time_point&    stop)
/*
Get the elapsed time as a nicely-formatted string (for writing to a file), e.g.
 - 11 hours 3 minutes 42 seconds
*/
{
    auto seconds_total = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
    size_t seconds = seconds_total % 60;
    size_t minutes = (seconds_total / 60) % 60;
    size_t hours   = seconds_total / 3600;
    std::ostringstream oss;
    if (hours > 0)
        oss << hours << (hours == 1 ? " hour " : " hours ");
    if (minutes > 0)
        oss << minutes << (minutes == 1 ? " minute " : " minutes ");
    oss << seconds << (seconds == 1 ? " second" : " seconds");
    return oss.str();
}

/**
****************************
Functions concerning spacing
****************************
**/

/**
template <typename T>
std::size_t get_fixed_width(// supplied data:
                            const T&    the_value)

Takes a numeric type value 'the_value' and works out how many digits there are in its string
representation.
**/

/**
template <typename T>
std::string truncate_or_pad(// supplied data:
                            const T&    a_value,
                            // supplied/computed data:
                            int         fixed_width)

Takes arithmetic 'a_value', ensures it is >=0, converts it to a string and either truncates it or pads it
with spaces so that it has size |'fixed_width'|:
 - if 'fixed_width' > 0 then padding is at the back/truncating from the back
 - if 'fixed_width' < 0 then padding is at the front/truncating from the front
 - if 'fixed_width' = 0 then nothing is done.
The new string is returned with the original left alone. The type T must be convertible to a string.
**/

/**
******************************************
Functions turning non-strings into strings
******************************************
**/

string get_a_percent_string(// supplied data:
                            const size_t    numerator,
                            const size_t    denominator)
/*
Build a percent string for printing from a numerator and a denominator.
*/
{
    string a_string;
    if (denominator == 0)                   // test division by 0
        return "div by 0!";
    double a_percent = 100 * double(numerator) / denominator;
    a_string = std::format("{:.{}f}", a_percent, System::dp_precision);
    return a_string + "%";
}

/**
*************************************
Functions concerning maps and vectors
*************************************
**/

void make_dense_vector(// supplied data:
                       const std::map<size_t, size_t>&    the_map,
                       // computed data:
                       vector<size_t>&                    map_as_vector)
/*
Returns a (load) map 'the_map' as a vector 'map_as_vector' so that the size of 'map_as_vector' is the
maximum key + 1 and 'map_as_vector[i]' = 'map[i]', if i is a key, and 'map_as_vector[i]' = 0 otherwise.
An empty map is returned as an empty vector.
*/
{
    map_as_vector.clear();
    if (the_map.empty())
        return;
    size_t max_key = the_map.rbegin()->first;
    map_as_vector.resize(max_key + 1, 0);
    for (const auto& [key, value] : the_map)
        map_as_vector[key] = value;
}

string format_double(// supplied data:
                     const double    value,
                     const size_t    decimals)
/*
Takes a double, sets it to a given number of decimal places and returns the result as a string.
*/
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    return oss.str();
}

/**
template <typename T>
std::string vector_to_a_string(// supplied data:
                               const std::vector<T>&    a_vector,
                               const int                spacing)

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
**/

