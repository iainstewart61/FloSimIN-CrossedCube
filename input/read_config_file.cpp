
#include <algorithm>
#include <cctype>
#include <fstream>

#include "read_config_file.h"
#include "../globals.h"

/*
The following are the config-related struct-headers with enums describing user-defined values.
*/

#include "../structs/structs_routing.h"
#include "../structs/structs_environment.h"
#include "../structs/structs_fault.h"

using std::string;
using std::vector;
using std::size_t;

/**
This cpp file contains all functions that read the config. The main function is:
 - read_config_file().
The config is read line-by-line where the config tag names:
 - <INTERCONNECTIONNETWORK>, <ROUTINGALGORITHM>, <TRAFFICPATTERN>, <FAULT>, <ENVIRONMENT>, <LATENCY>,
   <BANDWIDTH>, <PACKET>, <NOTES>.
are looked for with the read mode being dictated by the last tag name read (termination comes when we
find the tag <ENDOFFILE>). Within each read mode, we look for lines of the form:
 - prefix = suffix
where
 - prefix is a subtag name, namely a string corresponding to some global variable
 - suffix is a string containing a value intended for the global variable.
If the tag name is <TRAFFICPATTERN> or <FAULT> then we also look for traffic pattern descriptions and
fault descriptions, respectively.

Having found some appropriate prefix, we translate the suffix into a value of the appropriate type
(if it is valid) and check further restrictions that might be imposed on it. Then we give this value to
the appropriate global variable via:
 - register_value()
and any valid traffic pattern descriptions and fault descriptions are stored in the global variables:
 - TP.interim               via check_traffic_pattern()
 - FAU.fault_description    via check_fault_description().
After the config file has been read and processed, we check to see that core data has actually been
read from config.

The key data structure is:
 - config_tags
which includes information on:
 - the subtags associated with each tag
 - whether a tag or a subtag is core (that is, needs to appear in config)
 - whether a valid value (from suffix) has been read and set for the corresponding global variable or
   whether a tag has been encountered
 - any restrictions to be imposed when validating suffix
 - the (reference to the) global variable associated with the subtag name
 - the default value associated with the global variable (sometimes these are dummy and play no role).

Functions involving the assignment of values to globals:
 - assign_to_global()
 - set_defaults_of_globals()
Functions for parsing config:
 - check_for_read_mode_change()
Functions for filtering values w.r.t. restrictions:
 - filter_a_string()
 - filter_an_int()
 - filter_a_size_t()
 - filter_a_list_of_string()
 - filter_a_list_of_num()
Functions for getting standard-type values from a suffix:
 - get_a_string_from_a_suffix()
 - get_a_bool_from_a_suffix()
 - get_a_num_from_a_suffix()
 - get_a_list_of_string_from_a_suffix()
 - get_a_list_of_num_from_a_suffix()
Functions for getting user-defined-type values from a suffix:
 - is_allowable_string()
 - is_user_define()
 - get_a_user_type_from_a_suffix()
Functions for getting a named-list from a suffix:
 - get_a_named_list_of_size_t()
Register all values read from config:
 - register_value()
 - check_for_core()
Deal with potential traffic pattern descriptions:
 - validate_basic_tp()
 - lok_for_basic_tp()
 - look_for_tp_percentage()
 - string_is_prefixed_size_t()
 - string_is_prefixed_tuple()
 - look_for_sources_and_dests
 - check_traffic_pattern()
Deal with potential fault descriptions:
 - look_for_basic_fd()
 - look_for_fault_percentage_or_raw_count()
 - check_fault_description()
Read the config:
 - read_config_file()
**/

/**
*******************************************************
Functions involving the assignment of values to globals
*******************************************************
**/

void assign_to_global(// supplied data:
                      GlobalParamVal&          value,
                      GlobalParamRef&          ref_variant)
/*
Assigns a 'value' of type T to the global variable referenced via 'ref_variant'. Note that we need that the
type of the variable referenced by 'ref_variant' is also T. If we don't make an assignment then we flag this
as a coding error as 'value' and 'ref_variant' have come in with different underlying types.

Functions:
 - coding           from error_writing
*/
{
    bool undertaken = false;
    std::visit([&](auto& ref_wrapper)
    {
        using RefWrapper = std::decay_t<decltype(ref_wrapper)>;// omit e.g., &, *, const to leave raw type
        using T = typename RefWrapper::type;    // T is the underlying type of 'ref_variant'

        T& target = ref_wrapper.get();          // extract underlying global variable

        std::visit([&](auto& v)                 // assign only if types match exactly
        {
            using V = std::decay_t<decltype(v)>;// ditto for 'value'; so, 'v' has type T

            if constexpr (std::is_same_v<T, V>) // check types are identical
            {
                target = v;                     // if so then set the value of the global variable
                undertaken = true;              // register assignment
            }
            else
            {
                // do nothing: wrong variant alternative
            }

        }, value);
    }, ref_variant);
    if (!undertaken)                            // if no assignment made there is a coding problem
        coding(Errors::TypeMismatch, "a value for a global variable mismatches the type of the variable");
}

void set_defaults_of_globals(// supplied data:
                             vector<Tag>&    config_tags)
/*
Set the default values of all global variables using the data in 'config_tags'. Note that
'subtag.global_var' and 'subtag.default_value' always have the same type by construction. If something goes
wrong type-wise then 'assign_to_global' terminates with a coding error.

Functions:
 - assign_to_global         from here
*/
{
    for (auto& tag : config_tags)
    {
        for (auto& subtag : tag.subtags)
        {
            assign_to_global(subtag.default_value, subtag.global_var);
        }
    }
}

/**
****************************
Functions for parsing config
****************************
**/

Tag* check_for_read_mode_change(// supplied data:
                                const string&    stripped_string,
                                vector<Tag>&     config_tags)
/*
Check to see whether 'stripped_string' is a tag and if so, return a pointer to the actual tag in
'config_tags', with a nullptr returned otherwise (this allows us to test whether we have found a tag).
*/
{
    for (Tag& tag : config_tags)
    {
        if (tag.name == stripped_string)
            return &tag;
    }
    return nullptr;
}

Subtag* parse_for_prefix(// supplied data:
                         const string&    stripped_string,
                         Tag*             tag_ptr,
                         // computed data:
                         string&          suffix)
/*
Check to see whether 'stripped_string' contains a "=". If so then we check the prefix for legality and if
it is legal then we return a pointer to the corresponding subtag. We also compute 'suffix' as everything
to the right of "=" in 'stripped_string'. Note that 'tag_ptr' is always non-null (assumed). If the prefix
is not legal then we return nullptr (which allows us to check legality) and 'suffix' = "".
*/
{
    suffix.clear();
    size_t occurrence;
    occurrence = stripped_string.find("=");
    if (occurrence == string::npos)
        return nullptr;                             // there is no "="
    string prefix;
    prefix = stripped_string.substr(0, occurrence); // 'prefix' up to "="
    if ((occurrence + 1) < stripped_string.size())  // 'suffix' after "="
        suffix = stripped_string.substr(occurrence + 1);
    for (auto& subtag : tag_ptr->subtags)           // see if we have a subtag
    {
        if (subtag.name == prefix)                  // we have a legal prefix
            return &subtag;
    }
    suffix.clear();                                 // clear 'suffix'
    return nullptr;                                 // not a legal prefix
}

/**
**************************************************
Functions for filtering values w.r.t. restrictions
**************************************************
**/

Errors filter_a_string(//supplied data:
                       const string&    suffix,
                       Subtag*          prefix_ptr)
/*
Checks any restriction on 'suffix' w.r.t. 'prefix_ptr->restriction'.

Errors:
 - Good, EmptyString
*/
{
    if (prefix_ptr->restriction == Restrictions::NonEmpString)
    {
        if (suffix.empty())
            return Errors::EmptyString;
    }
    return Errors::Good;
}

Errors filter_an_int(//supplied data:
                     const int    an_int,
                     Subtag*      prefix_ptr)
/*
Checks any restriction on 'an_int' w.r.t. 'prefix_ptr->restriction'.

Errors:
 - Good, OutOfRangeInt
*/
{
    if (prefix_ptr->restriction == Restrictions::Gtr0Int)
    {
        if (an_int < 1)
            return Errors::OutOfRangeInt;
    }
    else if (prefix_ptr->restriction == Restrictions::GtrEq0Int)
    {
        if (an_int < 0)
            return Errors::OutOfRangeInt;
    }
    else if (prefix_ptr->restriction == Restrictions::GtrEqMinus1Int)
    {
        if (an_int < -1)
            return Errors::OutOfRangeInt;
    }
    return Errors::Good;
}

Errors filter_a_size_t(//supplied data:
                       const size_t    a_size_t,
                       Subtag*         prefix_ptr)
/*
Checks any restriction on 'a_size_t' w.r.t. 'prefix_ptr->restriction'.

Errors:
 - Good, OutOfRangeSize_t
*/
{
    if (prefix_ptr->restriction == Restrictions::Gtr0Size_t)
    {
        if (a_size_t == 0)
            return Errors::OutOfRangeSize_t;
    }
    return Errors::Good;
}

Errors filter_a_list_of_string(//supplied data:
                               vector<string>&    list_of_string,
                               const Subtag*      prefix_ptr)
/*
Checks any restriction on 'list_of_string' w.r.t. 'prefix_ptr->restriction'. As regards
'Restrictions::VecNonEmpFileString', if the strings don't end in '.txt' then they are amended.

Errors:
 - Good, EmptyStringInList
*/
{
    if (prefix_ptr->restriction == Restrictions::VecNonEmpFileString)
    {
        for (auto& a_string : list_of_string)
        {
            if (a_string.empty())
                return Errors::EmptyStringInList;
            if (a_string.size() < 4)
                a_string += ".txt";
            else if (a_string.substr(a_string.size() - 4) != ".txt")
                a_string += ".txt";
        }
    }
    else if (prefix_ptr->restriction == Restrictions::VecNonEmpString)
    {
        for (auto& a_string : list_of_string)
        {
            if (a_string.empty())
                return Errors::EmptyStringInList;
        }
    }
    return Errors::Good;
}

/**
template <typename T>
Errors filter_a_list_of_num(//supplied data:
                            const std::vector<T>&    list_of_num,
                            const Subtag*            prefix_ptr)

Checks any restriction on 'list_of_num' w.r.t. 'prefix_ptr->restriction'. Note that:
 - T must be size_t or uint32_t.

Errors:
 - Good, EmptyListOfSize_t, OutOfRangeListOfSize_t, BadLengthList
**/

/**
********************************************************
Functions for getting standard-type values from a suffix
********************************************************
**/

Errors get_a_string_from_a_suffix(// supplied data:
                                  const string&    suffix,
                                  // supplied/computed data:
                                  Subtag*          prefix_ptr)
/*
We filter the string 'suffix' for conditions and then set the value of the global variable corresponding to
the subtag to the value resurrected, otherwise we leave it alone (as it has been assigned its default value)
and just flag the error. Note that if we do set a global variable value then this is registered in
'config_tags'. We require that 'prefix_ptr->global_var' has type string and 'assign_to_global' will throw a
coding error if it doesn't and terminate.

Errors:
 - Good, EmptyString

Functions:
 - filter_a_string          from here
 - assign_to_global         from here
*/
{
    Errors error = filter_a_string(suffix, prefix_ptr);    // check restrictions on the string
    if (error == Errors::Good)
    {
        GlobalParamVal GPV_suffix = suffix;
        assign_to_global(GPV_suffix, prefix_ptr->global_var);
        prefix_ptr->value_read = true;
    }
    return error;
}

Errors get_a_bool_from_a_suffix(// supplied data:
                                const string&    suffix,
                                // supplied/computed data:
                                Subtag*          prefix_ptr)
/*
Try to resurrect a bool from 'suffix'. If we successfully resurrect a bool then we set the value of the
global variable corresponding to the subtag to this bool, otherwise we leave it alone (as it has been
assigned its default value) and just flag the error. Note that if we do set a global variable value then
this is registered in 'config_tags'. We require that 'prefix_ptr->global_var' has type bool and
'assign_to_global' will throw a coding error if it doesn't and terminate.

Errors:
 - Good, NotValidBool

Functions:
 - assign_to_global         from here
*/
{
    bool a_bool = true;
    if (suffix == "false")              // check whether 'suffix' is "false"
        a_bool = false;
    else if (suffix != "true")          // if 'suffix' != "true" ...
        return Errors::NotValidBool;    // ... return an error

    GlobalParamVal GPV_a_bool = a_bool;
    assign_to_global(GPV_a_bool, prefix_ptr->global_var);// assign value to global variable
    prefix_ptr->value_read = true;      // register that a value has been assigned

    return Errors::Good;
}

/**
template <typename T>
Errors get_a_num_from_a_suffix(// supplied data:
                               const string&    suffix,
                               // supplied/computed data:
                               Subtag*          prefix_ptr)

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
**/

Errors get_a_list_of_string_from_a_suffix(// supplied data:
                                          const string&    suffix,
                                          // supplied/computed data:
                                          Subtag*          prefix_ptr)
/*
Try to resurrect a list of strings from 'suffix'. After resurrecting a list, we filter for restrictions
before setting the global variable corresponding to the subtag to this list, otherwise we leave it alone
(as it has been assigned its default value) and just flag the error. Note that if we do set a global variable
value then this is registered in 'config_tags'. We require that 'prefix_ptr->global_var' has type
vector<string> and 'assign_to_global' will throw a coding error if it doesn't and terminate.

Errors:
 - Good, EmptyStringInList

Functions:
 - get_a_raw_list_of_string_from_a_suffix           from misc_string_functions
 - filter_a_list_of_string                          from here
 - assign_to_global                                 from here
*/
{
    Errors error;
    vector<string> list_of_string;
    get_a_raw_list_of_string_from_a_suffix(suffix, "[", "]", list_of_string);// convert to list of strings

    error = filter_a_list_of_string(list_of_string, prefix_ptr);// check restrictions on the list of strings
    if (error == Errors::Good)
    {
        GlobalParamVal GPV_list_of_string = list_of_string;
        assign_to_global(GPV_list_of_string, prefix_ptr->global_var);
        prefix_ptr->value_read = true;
    }
    return error;
}

/**
template <typename T>
Errors get_a_list_of_num_from_a_suffix(// supplied data:
                                       std::string&    suffix,
                                       // supplied/computed data:
                                       Subtag*         prefix_ptr)

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
**/

/**
************************************************************
Functions for getting user-defined-type values from a suffix
************************************************************
**/

Errors is_allowable_string(// supplied data:
                           const string&                    tag_name,
                           const string&                    subtag_name,
                           const string&                    string_value,
                           const vector<ValueAndTriple>&    values_and_triples,
                           // computed data:
                           GlobalParamVal&                  allowed_value)
/*
This function looks for {?, {'tag_name', 'subtag_name', 'value'}} in 'values_and_triples'. If no entry exists
then the error is returned as 'NotConAllowable'. If an entry does exist then we return the error as Good and
'allowed_value' as the user-defined value.

Errors:
 - Good, NotConAllowable
*/
{
    allowed_value = GlobalParamVal{};
    bool found = false;
    for (const auto& entry : values_and_triples)
    {
        if (entry.triple == TripleString{tag_name, subtag_name, string_value})
        {
            allowed_value = entry.value;
            found = true;
            break;
        }
    }
    if (found == false)
        return Errors::NotConAllowable;
    else
        return Errors::Good;
}

bool is_user_defined(// computed data:
                     Subtag*    prefix_ptr)
/*
Check whether the variable 'global_var' (being referred to) has a user-defined type.
*/
{
    if ((std::holds_alternative<std::reference_wrapper<RoutingDor>>(prefix_ptr->global_var))
        || (std::holds_alternative<std::reference_wrapper<RoutingTiebreak>>(prefix_ptr->global_var))
        || (std::holds_alternative<std::reference_wrapper<RoutingMode>>(prefix_ptr->global_var))
        || (std::holds_alternative<std::reference_wrapper<ViewRoutes>>(prefix_ptr->global_var))
        || (std::holds_alternative<std::reference_wrapper<ViewGlobals>>(prefix_ptr->global_var))
        || (std::holds_alternative<std::reference_wrapper<FaultCheck>>(prefix_ptr->global_var)))
        return true;
    else
        return false;
}

Errors get_a_user_type_from_a_suffix(// supplied data:
                                     const string&                    string_value,
                                     const Tag*                       tag_ptr,
                                     const vector<ValueAndTriple>&    values_and_triples,
                                     // supplied/computed data:
                                     Subtag*                          prefix_ptr)
/*
Try to resurrect a user-type value from {'tag_ptr->name', 'prefix_ptr->name', 'value'} within
'values_and_triples'. If we successfully resurrect a user-type value then we set the value of the global
variable corresponding to the subtag to this value, otherwise we leave it alone (as it has been assigned
its default value) and just flag the error. Note that if we do set a global variable value then this is
registered in 'config_tags'. We require that 'prefix_ptr->global_var' has the appropriate user-type and
'assign_to_global' will throw a coding error if it doesn't and terminate.

Errors:
 - Good, NotConAllowable

Functions:
 - is_allowable_string          from here
 - assign_to_global             from here
*/
{
    GlobalParamVal allowed_value;
    Errors error = is_allowable_string(tag_ptr->name, prefix_ptr->name, string_value, values_and_triples,
                                       allowed_value);
    if (error == Errors::Good)
    {
        assign_to_global(allowed_value, prefix_ptr->global_var);
        prefix_ptr->value_read = true;
    }
    return error;
}

/**
************************************************
Functions for getting a named-list from a suffix
************************************************
**/

Errors get_a_named_list_of_size_t(// supplied data:
                                  const string    &the_string,
                                  // supplied/computed data:
                                  Subtag*         prefix_ptr)
/*
Try to resurrect a named-list from 'suffix'. We do not set the global variables 'IN.param_names' and
'IN.param_values' via 'config_tags' but directly. Note that if we do set these global variable values
then this is registered in 'config_tags'.

Errors:
 - Good, NotValidNamedListOfSize_t

Functions:
 - look_for_delimiters          from misc_string_functions
 - split_string                 from misc_string_functions
*/
{
    vector<string> temp_list_of_strings;
    vector<string> another_temp_list_of_strings;
    size_t a_size_t;
    size_t pos_left;                                                        // left position
    size_t pos_right;                                                       // right position
    vector<string> list_of_names;                                           // list of param names
    list_of_names.reserve(std::count(the_string.begin(), the_string.end(), ',') + 1);
    vector<size_t> list_of_size_t;                                          // list of param values
    list_of_size_t.reserve(std::count(the_string.begin(), the_string.end(), ',') + 1);

    Errors error = look_for_delimiters(the_string, "[", "]", pos_left, pos_right);// look for [...]
    if (error != Errors::Good)
        return Errors::NotValidNamedListOfSize_t;
    string content = the_string.substr(pos_left + 1, pos_right - pos_left - 1); // get ... in [...]
    temp_list_of_strings = split_string(content, ",");                      // split
    for (const auto& an_item : temp_list_of_strings)                        // for each split string
    {
        another_temp_list_of_strings = split_string(an_item, "=");          // split each item around "="
        if (another_temp_list_of_strings.size() == 2)
        {
            try
            {
                size_t pos = 0;
                a_size_t = std::stoull(another_temp_list_of_strings[1], &pos);
                if (pos != another_temp_list_of_strings[1].size())
                    return Errors::NotValidNamedListOfSize_t;
                if (!another_temp_list_of_strings[0].empty())               // make sure names are non-empty
                {
                    if (a_size_t == 0)                                      // make sure integers are > 0
                        return Errors::NotValidNamedListOfSize_t;
                    list_of_names.push_back(another_temp_list_of_strings[0]);// save param name
                    list_of_size_t.push_back(a_size_t);                     // save param value
                }
                else
                    return Errors::NotValidNamedListOfSize_t;
            }
            catch (const std::invalid_argument&)                            // only two possible exceptions
            {
                return Errors::NotValidNamedListOfSize_t;
            }
            catch (const std::out_of_range&)
            {
                return Errors::NotValidNamedListOfSize_t;
            }
        }
        else
            return Errors::NotValidNamedListOfSize_t;
    }

    IN.param_names = list_of_names;                                         // set global variables
    IN.param_values = list_of_size_t;
    prefix_ptr->value_read = true;                                          // register successful reading
    return Errors::Good;
}

/**
************************************
Register all values read from config
************************************
**/

Errors register_value(// supplied data:
                      Tag*                             tag_ptr,
                      string&                          suffix,
                      const vector<ValueAndTriple>&    values_and_triples,
                      // supplied/computed data:
                      Subtag*                          prefix_ptr)
/*
Given the current tag pointer and the current prefix pointer:
 - try and resurrect a legal value from 'suffix'
 - if legal then set global variable and register changes.
Note that we handle "<INTERCONNECTIONNETWORK>"/"params" differently.

Errors:
 - Good, NotValidBool, EmptyString, NotValidInt, NotValidSize_t, OutOfRangeInt, OutOfRangeSize_t,
   NotValidListOfUint32_t, NotValidListOfSize_t, EmptyListOfSize_t, BadLengthList,
   NotValidNamedListOfSize_t, EmptyStringInList, NotConAllowable

Functions:
 - get_a_bool_from_a_suffix                     from here
 - get_a_string_from_a_suffix                   from here
 - get_a_num_from_a_suffix                      from here
 - get_a_list_of_num_from_a_suffix              from here
 - get_a_named_list_of_size_t                   from here
 - get_a_list_of_string_from_a_suffix           from here
 - get_a_user_type_from_a_suffix                from here
 - coding                                       from error_writing
*/
{
    Errors error;
    if (std::holds_alternative<std::reference_wrapper<bool>>(prefix_ptr->global_var))
        error = get_a_bool_from_a_suffix(suffix, prefix_ptr);
    else if (std::holds_alternative<std::reference_wrapper<string>>(prefix_ptr->global_var))
        error = get_a_string_from_a_suffix(suffix, prefix_ptr);
    else if (std::holds_alternative<std::reference_wrapper<int>>(prefix_ptr->global_var))
        error = get_a_num_from_a_suffix<int>(suffix, prefix_ptr);
    else if (std::holds_alternative<std::reference_wrapper<size_t>>(prefix_ptr->global_var))
        error = get_a_num_from_a_suffix<size_t>(suffix, prefix_ptr);
    else if (std::holds_alternative<std::reference_wrapper<vector<uint32_t>>>(prefix_ptr->global_var))
        error = get_a_list_of_num_from_a_suffix<uint32_t>(suffix, prefix_ptr);
    else if (std::holds_alternative<std::reference_wrapper<vector<size_t>>>(prefix_ptr->global_var))
        error = get_a_list_of_num_from_a_suffix<size_t>(suffix, prefix_ptr);
    else if (std::holds_alternative<std::reference_wrapper<vector<string>>>(prefix_ptr->global_var))
    {
        if ((tag_ptr->name == "<INTERCONNECTIONNETWORK>") && (prefix_ptr->name == "params"))
            error = get_a_named_list_of_size_t(suffix, prefix_ptr);
        else
            error = get_a_list_of_string_from_a_suffix(suffix, prefix_ptr);
    }
    else if (is_user_defined(prefix_ptr))
        error = get_a_user_type_from_a_suffix(suffix, tag_ptr, values_and_triples, prefix_ptr);
    else
        coding(Errors::TypeMismatch);
    return error;
}

Errors check_for_core(// supplied data:
                      vector<Tag>&    config_tags)
/*
Check that all core tags and subtags have been entered. If not then an error is signalled.

Errors:
 - Good, CoreTagMissing, CoreSubtagMissing
*/
{
    for (const auto& tag : config_tags)
    {
        if ((tag.core_tag == true) && (tag.value_read == false))
            return Errors::CoreTagMissing;
        for (const auto& subtag : tag.subtags)
        {
            if ((subtag.core_subtag == true) && (tag.value_read == false))
                return Errors::CoreSubtagMissing;
        }
    }
    return Errors::Good;
}

/**
************************************************
Deal with potential traffic pattern descriptions
************************************************
**/

Errors validate_basic_tp(// supplied stuff:
                         const vector<string>&     list_of_strings,
                         // computed stuff:
                         TrafficPatternInterim&    tp_interim)
/*
Validate the strings in 'list_of_strings' describing:
 - [name, #-iterations, #-repeats]
of a tp (we assume there are 1-3 strings) and store the validated values in 'tp_interim'. The only error that
can be returned here is when 'list_of_strings[0]' = "". Bad/missing data for #-iterations and #-repeats means
that 1 is plugged in (that is, defaults are retained). It is not possible to enter data for #-repeats but not
for #-iterations, i.e., a list of length 2 will be taken as name and #-iterations. So, on a good return:
 - 'tp_interim' has a non-empty name
 - #-iterations is a valid size_t > 0
 - #-repeats is a valid size_t > 0.
All that remains to do is to check that the name is a legitimate tp name, after the registry is read (of
course, the percentage, start-source, etc. have to be dealt with as well). We reiterate that when bad values
for #-iterations and #-repeats occur, we stick with the defaults of 1.

Errors:
 - Good, InvalidtpDesc

Functions:
 - string_to_value          from misc_string_functions
*/
{
    Errors error;
    size_t a_size_t;

    if (list_of_strings[0].empty())                     // no error unless name = ""
        return Errors::InvalidtpDesc;
    tp_interim.name = list_of_strings[0];               // set tp name (non-empty)
    if (list_of_strings.size() > 1)                     // there is another entry
    {
        error = string_to_value(list_of_strings[1], a_size_t);// convert second entry to size_t
        if ((error == Errors::Good) && (a_size_t > 0))
            tp_interim.num_iterations = a_size_t;
        if (list_of_strings.size() > 2)                 // if there is yet another entry
        {
            error = string_to_value(list_of_strings[2], a_size_t);// convert third entry to size_t
            if ((error == Errors::Good) && (a_size_t > 0))
                tp_interim.num_repeats = a_size_t;
        }
    }
    return Errors::Good;
}

Errors look_for_basic_tp(// supplied data:
                         const string&             line_from_a_file,
                         const string&             left_bookend,
                         const string&             right_bookend,
                         // computed data:
                         TrafficPatternInterim&    tp_interim)
/*
Looks for:
 - [name, #iterations, #-repeats]
and validates legitimacy. The only non-Good error message is when 'line_from_a_file' is ill-formed or there
is an empty tp name, in which case 'tp_interim' is the default. Successful validation means that 'tp_interim'
carries the legitimate values.

Errors:
 - Good, InvalidtpDesc

Functions:
 - look_for_delimiters          from misc_string_functions
 - split_string                 from misc_string_functions
 - validate_basic_tp            from here
*/
{
    size_t pos_left;
    size_t pos_right;
    Errors error;
    vector<string> list_of_strings;                     // the items between [...]

    error = look_for_delimiters(line_from_a_file, left_bookend, right_bookend, pos_left, pos_right);
    if (error == Errors::Good)                          // if there exists "[...]" ...
    {
        list_of_strings = split_string(line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1), ",");
        if (list_of_strings.size() > 3)                 // there already is at least 1 and need 1-3 items
            for (size_t i = 0; i < list_of_strings.size() - 3; ++i)
                list_of_strings.pop_back();             // prune extra strings
        error = validate_basic_tp(list_of_strings, tp_interim);// validate tp-name, #-iter, #-reps and store
        if (error != Errors::Good)
            tp_interim = TrafficPatternInterim{};
    }
    else
        error = Errors::InvalidtpDesc;
    return error;
}

Errors look_for_tp_percentage(// supplied data:
                              const string&             line_from_a_file,
                              const string&             left_bookend,
                              const string&             right_bookend,
                              // computed data:
                              TrafficPatternInterim&    tp_interim)
/*
Validates whether the percentage part of a tp is legitimate. There are no error messages and successful
validation means that the legitimate value is registered in 'tp_interim'. Note that in the case of a bad
value, the default is retained of 100%.

Errors:
 - Good, BadTpPercent

Functions:
 - look_for_delimiters          from misc_string_functions
 - string_to_value              from misc_string_functions
*/
{
    size_t pos_left;
    size_t pos_right;
    Errors error;
    size_t percentage;

    error = look_for_delimiters(line_from_a_file, left_bookend, right_bookend, pos_left, pos_right);
    if (error == Errors::Good)                       // if there exists "<...%>" (otherwise leave defaults)
    {
        error = string_to_value(line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1), percentage);
        if (error == Errors::Good)
        {
            if ((percentage > 0) && (percentage <= 100))// if good % size_t, set it otherwise leave default
                tp_interim.percentage = percentage;
        }
        else
            error = Errors::BadTpPercent;
    }
    else
        error = Errors::BadTpPercent;

    return error;
}

Errors string_is_prefixed_size_t(// supplied data:
                                 const string&    a_string,
                                 // computed data:
                                 string&          prefix,
                                 size_t&          a_size_t)
/*
Check 'a_string' to see if it is a prefixed size_t where the prefix is "pr" or "se". A good return results
in:
 - 'prefix' being "pr", se" or "*" ("*" denotes absid)
 - 'a_size_t' being a legitimate size_t.
If we do not find a good prefix then we treat the whole string as a potential (absid) size_t. A bad return
results in 'prefix' = "default", a_size_t = 0 and an error of BadPrefixedRelId is returned. Note that the
'a_size_t' needs to be checked later that it is in bounds (in validate_traffic_patterns() after the registry
has been read and topology_numerics() executed).

Errors:
 - Good, BadPrefixedRelId

Functions:
 - string_to_value          from misc_string_functions
*/
{
    Errors error;
    prefix = "*";
    string a_size_t_string = a_string;

    if (a_string.size() > 2)                            // only check for prefix if size >= 3
    {                                                   //  (|prefix| + 1 digit)
        for (size_t i = 0; i < G.num_term_types; ++i)   // if starts with "pr" or "se" then strip
        {
            if (a_string.substr(0,2) == G.node_types[i].short_name)
            {
                prefix = G.node_types[i].short_name;
                a_size_t_string = a_string.substr(2);
                break;
            }
        }
    }
    error = string_to_value(a_size_t_string, a_size_t); // try and convert string in hand to size_t
    if (error != Errors::Good)
    {
        prefix = "default";
        a_size_t = 0;
        return Errors::BadPrefixedRelId;
    }
    return Errors::Good;
}

Errors string_is_prefixed_tuple(// supplied data:
                                const string&      a_string,
                                // computed data:
                                string&            prefix,
                                vector<size_t>&    a_tuple)
/*
Check 'a_string' to see if it is a prefixed tuple where the prefix is "pr" or "se". A good return results in:
 - 'prefix' = "pr", "se" or "*" ("*" if there is no prefix and true prefix has to be decided later)
 - 'a_tuple' being a tuple of size_t.
If we do not find a good prefix then we will assume a prefix as the first terminal type "xx" for which node
type "xx" has >0 nodes and treat the whole string as a potential tuple. If 'a_string' is badly formatted then
 - 'prefix' = "default"
 - 'a_tuple' = empty.
Note that 'a_tuple' needs to be checked later as regards its length and its component values once the
registry has been read and topology_numerics() has been executed.

Errors:
 - Good, BadPrefixedAlgId

Functions:
 - look_for_delimiters          from misc_string_functions
 - split_string                 from misc_string_functions
 - string_to_value              from misc_string_functions
*/
{
    size_t pos_left;                                            // position of left delimiter
    size_t pos_right;                                           // position of right delimiter
    Errors error;
    size_t a_size_t;
    vector<string> a_tuple_of_strings;

    prefix = "*";
    a_tuple.clear();
    if (a_string.size() < 3)                                    // string is too short (cant be [...])
    {
        prefix = "default";
        return Errors::BadPrefixedAlgId;
    }

    string stripped_a_string = a_string;
    for (size_t i = 0; i < G.num_term_types; ++i)               // if starts with "pr" or "se" then strip
    {
        if (a_string.substr(0,2) == G.node_types[i].short_name)
        {
            prefix = G.node_types[i].short_name;
            stripped_a_string = a_string.substr(2);
        break;
        }
    }
    error = look_for_delimiters(stripped_a_string, "[", "]", pos_left, pos_right);// look for "[...]"
    if (error != Errors::Good)
    {
        prefix = "default";
        return Errors::BadPrefixedAlgId;                        // not well-formed
    }
    a_tuple_of_strings = split_string(stripped_a_string.substr(pos_left + 1, pos_right - pos_left - 1), ",");
    for (size_t i = 0; i < a_tuple_of_strings.size(); ++i)      // for each tuple component
    {
        error = string_to_value(a_tuple_of_strings[i], a_size_t);// try and convert stripped-prefix string
        if (error != Errors::Good)                              // if can't convert to size_t
        {
            prefix = "default";
            a_tuple.clear();
            return Errors::BadPrefixedAlgId;                    // not well-formed
        }
        a_tuple.push_back(a_size_t);                            // register (cleaned) string version of value
    }
    return Errors::Good;
}

Errors look_for_sources_and_dests(// supplied data:
                                  const string&             line_from_a_file,
                                  // computed data:
                                  TrafficPatternInterim&    tp_interim)
/*
An entry in the sources/destinations part of a tp can be:
 - default       : ends up as 0 for start node and 'G.num_terms' for destination node
    - 'prefix' = "default" and 'id_or_tuple' = undefined
 - pr34          : terminal node prefix followed by a relid
    - 'prefix' = "pr" and 'id_or_tuple' = 34
 - 45            : raw size_t denoting an absid
    - 'prefix' = "*" and 'id_or_tuple' = 45
 - pr[0,1,1,1,0] : terminal node prefix followed by an algid
    - 'prefix' = "pr" and 'id_or_tuple' = [0,1,1,1,0]
 - [0,1,1,1,1]   : algid where prefix is taken to be the first "xx" where 'G.node_type["xx"].number' > 0
    - 'prefix' = "*" and 'id_or_tuple' = [0,1,1,1,1]
Validates whether the sources and destinations part of a tp are legitimate. There are no error messages:
 - successful validation means that the legitimate values are registered in 'tp_interim'
 - unsuccessful means that the defaults are retained.
Note that the delimiter for sources and destinations is ";".

Errors:
 - Good, NoSourcesDests

Functions:
 - look_for_delimiters                  from misc_string_functions
 - split_string                         from misc_string_functions
 - string_is_prefixed_size_t            from here
 - string_is_prefixed_tuple             from here
*/
{
    Errors error;
    size_t pos_left;                                            // location of "{"
    size_t pos_right;                                           // location of "}"
    vector<string> list_of_strings;                             // the list of sources and destinations
    string prefix;                                              // a prefix of a source or destination
    size_t a_size_t;                                            // a value describing a node
    vector<size_t> a_tuple;                                     // a tuple describing a node

    error = look_for_delimiters(line_from_a_file, "{", "}", pos_left, pos_right);// look for "{...}"
    if (error == Errors::Good)                                  // if "{...}" ... (if not leave as default)
    {
        list_of_strings = split_string(line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1), ";");
        if (list_of_strings.size() > 4)
            for (size_t j = 0; j < list_of_strings.size() - 4; ++j)
                list_of_strings.pop_back();                     // prune excess strings
        if (list_of_strings.size() < 4)
            for (size_t j = 0; j < 4 - list_of_strings.size(); ++j)
                list_of_strings.push_back("default");           // too few strings so add on "default"
        for (size_t j = 0; j < 4; ++j)                          // now 4 strings: process each of them
        {
            if (list_of_strings[j] != "default")                // leave alone if "default"
            {
                error = string_is_prefixed_size_t(list_of_strings[j], prefix, a_size_t);
                if (error == Errors::Good)                      // if we have found a prefixed size_t
                {
                    tp_interim.sources_dests[j].prefix = prefix;// register the prefix
                    tp_interim.sources_dests[j].id_or_tuple = a_size_t;// register the size_t
                }
                else
                {
                    error = string_is_prefixed_tuple(list_of_strings[j], prefix, a_tuple);
                    tp_interim.sources_dests[j].prefix = prefix;// register the prefix
                    tp_interim.sources_dests[j].id_or_tuple = a_tuple;// register (non-empty) tuple
                }
            }
        }
    }
    else
        error = Errors::NoSourcesDests;

    return error;
}

Errors check_traffic_pattern(// supplied_data:
                             const string&    line_from_a_file)
/*
The string in 'line_from_a_file' has already been stripped. An expected tp entry is of the form:
 - [tp-name, #-iterations, #-repeats]<percentage-batch%>{start-source; end-source; start-dest; end-dest}
as a string where the start-source, end-source, etc., values are strings of the form:
 - 31 as an absid or, e.g., se31 to denote the server with relid 31
      we need to check later that:
       - the resulting absid is within the appropriate node-type range
 - [a,b,c,d] or e.g., pr[a,b,c,d] to denote the processor with algid [a,b,c,d] (see below)
      (note that this tuple is only checked to see that it is a tuple of non-neg int here:
       - the actual length of the tuple
       - the values of the tuple
      need to be checked later after the registry has been read and topology_numerics() has been executed)
 - "default",
      whereby we will later plug in:
       - 0 for a start-source/dest
       - 'G.num_terms' for an end-source/dest
       (the value of end-source/-dest will be 1 + actual end-source/dest: see validate_traffic_patterns()).
Note that the delimiter for start-source, end-source, etc., is ";" as it may be the case that commas are used
in algid representations.

The prefix "pr" or "se" can be present or absent re: tuples:
 - if absent then it is assumed to be the first node short-name type "xx" for which 'G.num_nodes[G.xx]' > 0.
We can omit #-iterations or #-repeats when defaults of 1 are assumed:
 - if there is one param then it is #-iterations.
We can omit <percentage-batch%> when 100% is assumed (percentage-batch is expected to be a size_t). We can
omit all of {start-source; end-source; start-dest; end-dest} or just include some values. The values that are
there are read from the left with any omitted substituted with "default". For;
 - <percentage-batch%>
 - {start-source; end-source; start-dest; end-dest},
ill-formed entries are not flagged as errors but are set to the default.

A valid entry is repackaged here as a TrafficPatternInterim struct. When this repackaged entry is later
expanded and validated in validate_traffic_patterns(), it will result in a sequence of traffic patterns of
struct TrafficPatternExpanded. Any, e.g., start-source will be the first source node and (end-source - 1)
will be the last source node. We will also ensure that both the start-range and the dest-range contain at
least one node (if not then 0 and 'G.num_terms' are substituted). However, only the following checks are made
here in the struct TrafficPatternInterim:
 - name = tp-name != ""
 - num_iterations > 0
 - num_repeats > 0
 - percentage > 0 and <= 100
 - each of the 4 entries of sources_dests is s.t.
   - prefix = "pr", "se", "*" or "default"
   - id_or_tuple = a size_t or vector<size_t>:
      - if prefix = "default" then this value will be empty
      - if prefix = "pr", "se" or "*" then relative id size_t or vector<size_t>.
The intention will be that:
 - if prefix = "*" and size_t then absid
 - if prefix = "*" and vector<size_t> then relid for first "xx" where number of nodes of "xx" is > 0.
This will be evaluated in validate_traffic_patterns().

All entries are stored in the global TrafficPattern struct variable TP via TP.interim.

Errors:
 - Good, InvalidtpDesc, BadTpPercent

Functions:
 - look_for_basic_tp                        from here
 - look_for_tp_percentage                   from here
 - look_for_sources_and_dests               from here
*/
{
    Errors error;
    TrafficPatternInterim tp_interim{};     // tp with defaults;

    error = look_for_basic_tp(line_from_a_file, "[", "]", tp_interim);// check for [tp-name, #-iter, #-reps]
    if (error == Errors::Good)                                        //  and store in 'tp_interim'
    {
        error = look_for_tp_percentage(line_from_a_file, "<", "%>", tp_interim);// validate <...%> and store
        error = look_for_sources_and_dests(line_from_a_file, tp_interim);// validate {...} and store
        TP.interim.push_back(tp_interim);   // store 'tp_interim'
        return error;
    }
    else
        return Errors::InvalidtpDesc;
}

/**
**************************************
Deal with potential fault descriptions
**************************************
**/

Errors look_for_basic_fd(// supplied data:
                         const string&        line_from_a_file,
                         const string&        left_bookend,
                         const string&        right_bookend,
                         // supplied/computed data:
                         FaultDescription&    fault_desc)
/*
Validates whether the:
 - [name, node-type1, node-type2]-part of a fault description
appears in 'line_from_a_file'. An error only comes about if the name is not "node", "channel" or "link" when
we stick with defaults, i.e., "*". Success means that the legitimate values are registered in 'fault_desc'
(which comes in as the default).

Errors:
 - Good, InvalidFaultDesc

Functions:
 - look_for_delimiters          from misc_string_functions
 - split_string                 from misc_string_functions
*/
{
    size_t pos_left;
    size_t pos_right;
    Errors error = Errors::Good;
    vector<string> list_of_strings;                     // the items between [...] if found

    error = look_for_delimiters(line_from_a_file, left_bookend, right_bookend, pos_left, pos_right);
    if (error == Errors::Good)                          //  look for "[...]" and if there exists "[...]"
    {
        list_of_strings = split_string(line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1), ",");
        if ((list_of_strings[0] != "node") && (list_of_strings[0] != "channel")
            && (list_of_strings[0] != "link"))
            return Errors::InvalidFaultDesc;
        fault_desc.name = list_of_strings[0];           // set valid name
        if (list_of_strings.size() > 1)                 // check if valid short name (only if more params)
        {
            for (size_t i = 0; i < G.num_node_types; ++i)
            {
                if (list_of_strings[1] == G.node_types[i].short_name)
                {
                    fault_desc.source_node_type = list_of_strings[1];// set valid source_node_type
                    break;
                }
            }
        }
        if ((list_of_strings.size() > 2) && (list_of_strings[0] != "node"))// check if valid short name
        {                                                                  //  for channels or links
            for (size_t i = 0; i < G.num_node_types; ++i)
            {
                if (list_of_strings[2] == G.node_types[i].short_name)
                {
                    fault_desc.dest_node_type = list_of_strings[2];     // set valid dest_node_type
                    break;
                }
            }
        }
        return Errors::Good;
    }
    else
        return Errors::InvalidFaultDesc;
}

Errors look_for_fault_percentage_or_raw_count(// supplied data:
                                              const string&        line_from_a_file,
                                              const string&        left_bookend,
                                              const string&        right_bookend,
                                              // supplied/computed data:
                                              FaultDescription&    fault_desc)
/*
Validates whether the percentage/raw count part of a fault description is legitimate. Note that it checks
for a percentage in <...%> or a raw count in <...>. If we are successful then 'fault_desc' is updated,
otherwise it is untouched and an error is signalled. Note that if we have a percentage then this percentage
must be > 0% and < 100% and if we have a raw count then this count must be > 0.

Errors:
 - Good, IllFormedNumberFault, NumberFaultOutOfBounds

Functions:
 - look_for_delimiters          from misc_string_functions
 - string_to_value              from misc_string_functions
*/
{
    Errors error;
    size_t pos_left;
    size_t pos_right;
    bool a_percent = false;                                 // whether we are looking for a percentage
    double a_double;
    size_t a_size_t;

    error = look_for_delimiters(line_from_a_file, left_bookend, right_bookend, pos_left, pos_right);
    if (error == Errors::Good)                              // look for "<...>" and if exists ...
    {
        if (pos_right - pos_left > 1)                       // if there is a char between '<' and '>'
        {
            if (line_from_a_file[pos_right - 1] == '%')     // test for %
            {
                a_percent = true;
                pos_right--;                                // omit % sign
            }
        }
        else                                                // only have '<>'
            return Errors::IllFormedNumberFault;
        if (a_percent == true)                              // if its a percent
        {
            error = string_to_value(line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1), a_double);
            if (error == Errors::Good)
            {
                if ((a_double > 0) && (a_double < 100))     // ensure > 0 and < 100 (otherwise ignore)
                    fault_desc.percentage = a_double;
                else
                    return Errors::NumberFaultOutOfBounds;
            }
            else                                            // badly formatted fault percentage number
                return Errors::IllFormedNumberFault;
        }
        else                                                // if its a raw count
        {
            error = string_to_value(line_from_a_file.substr(pos_left + 1, pos_right - pos_left - 1), a_size_t);
            if (error == Errors::Good)
            {
                if (a_size_t > 0)                           // check > 0
                    fault_desc.raw_count = a_size_t;
                else
                    return Errors::NumberFaultOutOfBounds;
            }
            else                                            // badly formatted fault raw count number
                return Errors::IllFormedNumberFault;
        }
    }
    return Errors::Good;
}

Errors check_fault_description(// supplied data:
                                    const string&    line_from_a_file)
/*
A string entry in config for a fault description is of the form:
 - ["node", node-type]<x> or <x\%> - x or x% faulty nodes of node type node-type, e.g., "pr", "sw", "*"
   where "*" denotes all nodes
 - ["channel", node-type1, node-type2]<x> or <x\%> - x or x% faulty channels of channel type <node-type1,
   node-type2>
 - ["link", node-type1, node-type2]<x> or <x\%> - x or x% faulty links of paired channels of type
   <node-type1, node-type2> and <node-type2, node-type1>.
If node-type1 or node-type2 is omitted or not in {"pr", "se", "sw", "os", "*"} then the node-type is
registered as "*". For channel or link faults where one node-type is omitted, "*" is assumed as the second
node-type. Note that it only makes sense to set links as faulty when the interconnection network is a full
duplex undirected graph.

The data from a valid fault description is saved as a struct FaultDescription:
 - name = string "node", "channel" or "link"
 - percentage = 0 <= double < 100
 - raw_count = size_t >= 0
 - source_node_type = node type in {"pr", "se", "sw", "os", "*"}
 - dest_node_type = node type in {"pr", "se", "sw", "os", "*"}
 - number_of_faults = aimed for number of faults to be set.
Exactly one of percentage and raw_count is 0 in a valid fault description. Any valid fault description is
stored in 'FAU.fault_description'.

Note that later we will need to:
 - compute the actual number of intended 'raw_count'/'percentage' faults to be set
 - check that this number is < the appropriate number of nodes, channels or links and > 0
 - if valid, store this in 'FAU.fault_descriptionnumber_of_faults'.
This is done via validate_fault_descriptions().

Errors:
 - Good, InvalidFaultDesc, IllFormedNumberFault, NumberFaultOutOfBounds

Functions:
 - look_for_basic_fd                                from here
 - look_for_fault_percentage_or_raw_count           from here
*/
{
    Errors error;
    FaultDescription fault_desc{};

    error = look_for_basic_fd(line_from_a_file, "[", "]", fault_desc);
    if (error == Errors::Good)
    {
        error = look_for_fault_percentage_or_raw_count(line_from_a_file, "<", ">", fault_desc);
        if (error == Errors::Good)
            FAU.fault_description.push_back(fault_desc);
        return error;
    }
    else
        return error;
}

/**
***************
Read the config
***************
**/

void read_config_file(// supplied data:
                      const string&              input_file_name,
                      // computed data:
                      vector<Tag>&               config_tags,
                      vector<ValueAndTriple>&    values_and_triples,
                      // other stuff:
                      std::ostream&              output)
/*
Read the config and:
 - set global variable values
 - register traffic patterns
 - register fault descriptions.

Functions:
 - fatal                                from error_writing
 - create_config_tags                   from data_structures
 - create_values_and_triples            from data_structures
 - set_defaults_of_globals              from here
 - strip_string                         from misc_string_functions
 - check_for_read_mode_change           from here
 - parse_for_prefix                     from here
 - get_spacey_suffix                    from misc_string_functions
 - register_value                       from here
 - check_traffic_pattern                from here
 - check_fault_description              from here
 - check_for_core                       from here
*/
{
    Errors error;
    bool found_eof = false;

    // open config

    std::ifstream the_file(input_file_name);    // input file stream 'input_file_name'
    if (!the_file)                              // if there is something wrong then terminate
        fatal(Errors::ProblematicFile, input_file_name);
    output << "\n'" << input_file_name << "' has been opened\n";

    // create config data structures and set defaults for globals

    output << "\nsetting up config data structures and defaults for globals ...\n";

    config_tags = create_config_tags();
    values_and_triples = create_values_and_triples();
    set_defaults_of_globals(config_tags);

    output << "... config data structures and defaults for globals set up\n";

    // read config

    output << "\nreading config line by line ...\n";

    string line_string;                         // a line from 'input_file_name'
    Tag* tag_ptr = nullptr;                     // points to active tag
    while (std::getline(the_file, line_string)) // read the input file line by line
    {
        // strip the line

        string stripped_string = strip_string(line_string);// strip the string
        if (stripped_string.empty())
            continue;

        if (stripped_string == "<ENDOFFILE>")   // if the file has been read then quit
        {
            output << "\n<ENDOFFILE>\n";
            found_eof = true;
            break;
        }

        // check whether a new tag has been found

        Tag* new_tag_ptr = check_for_read_mode_change(stripped_string, config_tags);
        if (new_tag_ptr == nullptr)             // if there is no read mode change ...
        {
            if (tag_ptr == nullptr)             // if we have yet to find a tag then try again
                continue;
            Subtag* prefix_ptr = nullptr;       // points to subtag
            string suffix;

            // look for a prefix and process the value

            prefix_ptr = parse_for_prefix(stripped_string, tag_ptr, suffix);// look for suitable prefix
            if (prefix_ptr != nullptr)          // if a valid prefix found ...
            {
                if ((tag_ptr->name == "<NOTES>") && (prefix_ptr->name == "added_note"))
                    get_spacey_suffix(line_string, prefix_ptr->name, suffix, stripped_string);// never error
                output << stripped_string << "\n";
                error = register_value(tag_ptr, suffix, values_and_triples, prefix_ptr);
                if (error != Errors::Good)
                    output << "  " << short_warning(error, "line ignored") << "\n";
            }
            else                                // no prefix so look for tps or faults
            {
                if (tag_ptr->name == "<TRAFFICPATTERN>")
                {
                    Errors error;
                    error = check_traffic_pattern(stripped_string);// get a tp and store it if successful
                    output << stripped_string << "\n";
                    if (error == Errors::InvalidtpDesc)
                        output << "  " << short_warning(error, "line ignored") << "\n";
                }
                else if (tag_ptr->name == "<FAULT>")
                {
                    Errors error;
                    error = check_fault_description(stripped_string);// get a fd and store it if successful
                    output << stripped_string << "\n";
                    if (error == Errors::InvalidFaultDesc)
                        output << "  " << short_warning(error, "line ignored") << "\n";
                }
            }
        }
        else                                    // ... else there is a change of read mode
        {
            output << "\n" << stripped_string << "\n";
            new_tag_ptr->value_read = true;     // register that a tag has been read
            tag_ptr = new_tag_ptr;              // register new tag
        }
    }

    if (the_file.bad())
        fatal(Errors::ProblematicFile, input_file_name);
    if (the_file.fail() && !the_file.eof())
        fatal(Errors::ProblematicFile, "error while reading file");
    if (!found_eof)
        fatal(Errors::NotViaEOF);
    output << "\n... '" << input_file_name << "' has been read, processed and closed\n";

    error = check_for_core(config_tags);
    if (error != Errors::Good)
        fatal(error);
}
