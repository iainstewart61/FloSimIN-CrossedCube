
#include <algorithm>

#include "misc_string_functions.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp contains miscellaneous functions involving string manipulations.

- strip_string()
- look_for_delimiters()
- split_string()
- string_to_value()
- get_a_raw_list_of_string_from_a_suffix()
- build_list_of_num_from_strings()
- get_spacey_suffix()
**/

string strip_string(// supplied/computed data:
                    const string&    the_string)
/*
Removes all characters after and including the first '#' and subsequently removes spaces and controls from
'the_string'.
*/
{
    string stripped_string = the_string;
    size_t pos_hash = stripped_string.find('#');    // look for '#'
    if (pos_hash != string::npos)                   // if a '#' exists ...
        stripped_string.erase(pos_hash);            // ... erase it and everything following
    stripped_string.erase(std::remove_if(stripped_string.begin(), stripped_string.end(),// remove controls and
        [](unsigned char c){return std::iscntrl(c) || std::isspace(c);}), stripped_string.end());// spaces
    return stripped_string;
}

Errors look_for_delimiters(// supplied data:
                           const string&    the_string,
                           const string&    left_bookend,
                           const string&    right_bookend,
                           // computed data:
                           size_t&          pos_left,
                           size_t&          pos_right)
/*
Finds the positions of 'left_bookend' and 'right_bookend', if they occur.

Errors:
 - Good, MissingRightBookend, MissingLeftBookend

Functions:
 - coding               from error_writing
*/
{
    pos_left = pos_right = string::npos;
    if (left_bookend.empty() || right_bookend.empty())
        coding(Errors::EmptyDelimiter);         // bad coding so terminate
    pos_left = the_string.find(left_bookend);   // left pos. if exists
    if (pos_left != string::npos)               // if the left pos. exists ...
    {
        pos_right = the_string.find(right_bookend, pos_left + left_bookend.size());// right pos. if exists
        if (pos_right != string::npos)          // if the right pos. exists ...
            return Errors::Good;                // ... found bookends
        else                                    // if the right pos. does not exist ...
            return Errors::MissingRightBookend; // ... right bookend missing
    }
    return Errors::MissingLeftBookend;          // left bookend missing
}

vector<string> split_string(// supplied data:
                            const string&    the_string,
                            const string&    delimiter)
/*
Splits a string around 'delimiter'. A non-empty vector of strings is always returned (even if
'the_string' = "").
*/
{
    if (delimiter.empty())
        coding(Errors::MissingDelimiter); // bad coding so terminate
    vector<string> list_of_strings;
    size_t the_start = 0;               // position to start looking from
    size_t the_end;                     // position of next occurrence
    while ((the_end = the_string.find(delimiter, the_start)) != std::string::npos)
    {
        list_of_strings.push_back(the_string.substr(the_start, the_end - the_start));// add on string
        the_start = the_end + delimiter.size();// register a new start position
    }
    if (the_start >= the_string.size())
        list_of_strings.push_back("");  // the last 'delimiter' fills the last characters of 'the_string'
    else
        list_of_strings.push_back(the_string.substr(the_start));// last string found is non-empty
    return list_of_strings;
}

/**
template<typename T>
Errors string_to_value(// supplied data:
                       const std::string&    suffix,
                       // computed data:
                       T&                    value)

Converts 'suffix' to 'value' with:
 - T being size_t, double or int.
We check that 'suffix' does not start with '-' for size_t as this leads to wrap-around difficulties. Note
that for 'suffix' to be valid, all of the characters must contribute to the size_t, double or int.

Errors:
 - Good, NotValidInt, NotValidSize_t, NotValidDouble, OutOfRangeInt, OutOfRangeSize_t, OutOfRangeDouble
**/

void get_a_raw_list_of_string_from_a_suffix(// supplied data:
                                            const string&      suffix,
                                            const string&      left_bookend,
                                            const string&      right_bookend,
                                            // computed data:
                                            vector<string>&    list_of_string)
/*
Resurrect a list of strings from 'suffix'. If 'suffix' does not include 'left_bookend' and 'right_bookend'
then the list resurrected is ['suffix']. No check is made on the strings in the list (they might be empty).

Functions:
 - look_for_delimiters          from here
 - split_string                 from here
*/
{
    list_of_string.clear();
    size_t pos_left;
    size_t pos_right;
    Errors error = Errors::Good;

    error = look_for_delimiters(suffix, left_bookend, right_bookend, pos_left, pos_right);
    if (error == Errors::Good)                                  // 'suffix' contains a list
    {
        string inner = suffix.substr(pos_left + 1, pos_right - pos_left - 1);// extract the contents
        list_of_string = split_string(inner, ",");              // get list of strings
    }
    else                                                        // the string does not contain a list
    {
        list_of_string = {suffix};
    }
}

/**
template <typename T>
Errors build_list_of_num_from_strings(// supplied data:
                                      const std::vector<std::string>&    list_of_strings,
                                      // computed data:
                                      std::vector<T>&                    list_of_num)

Builds a list of T from a list of strings with:
 - T being size_t or uint32_t.
If there is an error then it is signalled and the empty list is returned. Note that it is possible that
'list_of_strings' is empty when Good will be returned along with an empty 'list_of_num'.

Errors:
 - NotValidListOfUint32_t, NotValidListOfSize_t, OutOfRangeListOfUint32_t, OutOfRangeListOfSize_t
**/

void get_spacey_suffix(// supplied data:
                       const string&    line_string,
                       const string&    prefix,
                       //computed data:
                       string&          suffix,
                       string&          stripped_string)
/*
We know for sure that 'line_string' starts "prefix" + "=" but possibly with spaces/controls thrown in.
Take 'line_string' (which is the line string including spaces/controls) and:
 - find where "=" is
 - remove everything up to this char along with trailing spaces
 - set the result as 'suffix'
 - set 'stripped_string' as 'prefix' + "= " + 'suffix'.
Note that we never get an error from this function.
*/
{
    size_t pos = line_string.find('=');
    ++pos;

    while (pos < line_string.size())    // work through 'line_string' skipping spaces/controls
    {
        unsigned char ch = (unsigned char)line_string[pos];
        if (std::iscntrl(ch) || std::isspace(ch))
            ++pos;
        else
            break;
    }
    if (pos == line_string.size())
        suffix = "";
    else
        suffix = line_string.substr(pos);
    stripped_string = prefix + " = " + suffix;
}
