
#ifndef MISC_DATA_STRUCTURES_H
#define MISC_DATA_STRUCTURES_H

#include <tuple>
#include <vector>
#include <string>
#include <array>
#include <variant>
#include <functional>
#include <compare>
#include <cstddef>
#include <cstdint>

/*
The following are the struct-headers with enums describing user-defined values (see 'GlobalParamTypes'
below).
*/

#include "../structs/structs_int_net.h"
#include "../structs/structs_routing.h"
#include "../structs/structs_environment.h"
#include "../structs/structs_fault.h"
#include "../structs/structs_registry.h"

/*
Convert a std::tuple<Ts...> into std::variant<Ts...>.
*/
template <typename Tuple>
struct VariantFromTuple;

template <typename... Ts>
struct VariantFromTuple<std::tuple<Ts...>>
{
    using type = std::variant<Ts...>;
};

/*
Convert a std::tuple<Ts...> into std::variant<std::reference_wrapper<Ts>...>.
*/
template <typename Tuple>
struct RefVariantFromTuple;

template <typename... Ts>
struct RefVariantFromTuple<std::tuple<Ts...>>
{
    using type = std::variant<std::reference_wrapper<Ts>...>;
};

/*
List of all supported parameter types (compile-time type list). Used to generate variants for
values and references. These types arise when inputting values from the config for parameters
corresponding to global variables or from the registry for parameters relating to properties of
interconnection networks.
*/
using GlobalParamTypes = std::tuple
<
    // config

    bool,
    int,
    std::size_t,
    std::string,
    std::vector<std::size_t>,
    std::vector<std::uint32_t>,
    std::vector<std::string>,
    RoutingDor,
    RoutingTiebreak,
    RoutingMode,
    ViewRoutes,
    ViewGlobals,
    FaultCheck,

    // registry

    RegistryNetMachine,
    RegistryNetType,
    RegistryNetBuild
>;

/*
Holds values of supported parameter types. Used for storing defaults, parsed config values, etc.
*/
using GlobalParamVal = typename VariantFromTuple<GlobalParamTypes>::type;

/*
Holds references to global variables of supported types. Used to bind config parameters to actual global
variables.
*/
using GlobalParamRef = typename RefVariantFromTuple<GlobalParamTypes>::type;

enum class Restrictions
/*
Restrictions on data (of different types and self-explanatory) that is read from config.
*/
{
    None,
    // numeric
    Gtr0Int, GtrEq0Int, GtrEqMinus1Int, Gtr0Size_t,
    // string
    NonEmpString,
    // vector of num
    NonEmpVecGtr0Size_t, Vec4Size_t, Vec16Size_t, Vec8Uint32_t,
    // vector of strings
    VecNonEmpFileString, VecNonEmpString
};

struct Subtag
/*
A struct for a member of 'subtags' of a struct 'Tag'.
 - name             the name of the associated parameter corresponding to some global variable that is
                    covered by the 'Tag' structure
 - core_subtag      whether the subtag is a core subtag whose name has to appear in the config
 - value_read       whether a value for the associated parameter has been read from the config
 - restriction      the restriction on the value for the associated parameter (bearing in mind the type
                    of the corresponding global variable)
 - global_var       the global variable that is associated with the parameter
 - default_value    the default value of the corresponding global variable
*/
{
    std::string             name;
    bool                    core_subtag = false;
    bool                    value_read = false;
    Restrictions            restriction = Restrictions::None;
    GlobalParamRef          global_var;
    GlobalParamVal          default_value;
};

struct Tag
/*
A struct for a tag of the 'config_tags' data structure.
 - name             the tag name of the form "<name-of-tag>" as it should appear in the config
 - core_tag         whether the tag is a core tag whose name has to appear in the config
 - value_read       whether the associated tag has been read from the config
 - subtags          a vector of 'Subtag' structures, one for each global variable covered by this tag
*/
{
    std::string             name;
    bool                    core_tag = false;
    bool                    value_read = false;
    std::vector<Subtag>     subtags;
};

struct TripleString
/*
A struct describing a list of three strings for 'values_and_triples'.
*/
{
    std::string tag_name;
    std::string subtag_name;
    std::string value;

    auto operator<=>(const TripleString&) const = default;
};

struct ValueAndTriple
/*
A struct tying user-defined values for global variables with triples consisting of:
 - main tag name
 - subtag name
 - string describing the user-defined value.
*/
{
    GlobalParamVal    value;
    TripleString      triple;
};

std::vector<Tag> create_config_tags();

std::vector<ValueAndTriple> create_values_and_triples();

#endif
