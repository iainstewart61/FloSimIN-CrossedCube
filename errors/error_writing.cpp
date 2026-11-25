
#include <iostream>
#include <stdexcept>

#include "error_writing.h"

using std::string;
using std::unordered_map;
using namespace std::string_literals;

/**
This cpp files contains all error messages, their expansions and the functions to display these
error messages.

Functions:
 - format_error()
 - fatal()
 - coding()
 - warning()
 - short_warning()
**/

const std::unordered_map<Errors, std::string> error_messages =
{
    {Errors::Good, "No error"},

    // config and registry: string manipulation
    {Errors::EmptyString, "a string is empty"},
    {Errors::MissingDelimiter, "there is a missing delimiter"},
    {Errors::MissingLeftBookend, "there is a missing left book-end"},
    {Errors::MissingRightBookend, "there is a missing right book-end"},
    {Errors::NotValidBool, "there is an invalid boolean"},
    {Errors::NotValidDouble, "there is an invalid double"},
    {Errors::NotValidInt, "there is an invalid integer"},
    {Errors::NotValidSize_t, "there is an invalid size_t"},

    // config and registry: list manipulation
    {Errors::BadLengthList, "a list of items has the wrong length"},
    {Errors::EmptyListOfSize_t, "there is an empty list of size_t"},
    {Errors::EmptyStringInList, "there is an empty string in a list"},
    {Errors::NotValidListOfSize_t, "there is an invalid list of size_t"},
    {Errors::NotValidListOfUint32_t, "there is an invalid list of uint32_t"},
    {Errors::NotValidNamedListOfSize_t, "there is an invalid named list of size_t"},

    // config and registry: out of range values
    {Errors::OutOfRangeDouble, "there is an out-of-range double"},
    {Errors::OutOfRangeInt, "there is an out-of-range integer"},
    {Errors::OutOfRangeListOfSize_t, "there is an out-of-range list of size_t"},
    {Errors::OutOfRangeListOfUint32_t, "there is an out-of-range list of uint32_t"},
    {Errors::OutOfRangeSize_t, "there is an out-of-range size_t"},

    // config: traffic pattern errors
    {Errors::BadPrefixedAlgId, "there is a badly prefixed algebraic id"},
    {Errors::BadPrefixedRelId, "there is a badly prefixed relative id"},
    {Errors::BadTpPercent, "there is a badly formatted percentage"},
    {Errors::InvalidtpDesc, "there is an invalid traffic pattern description"},
    {Errors::NoSourcesDests, "no sources and destinations have been included"},

    // config: fault errors
    {Errors::IllFormedNumberFault, "the intended number of faults is ill-formed"},
    {Errors::InvalidFaultDesc, "there is a invalid fault description"},
    {Errors::NumberFaultOutOfBounds, "the intended number of faults is out of bounds"},

    // config: user-defined values errors
    {Errors::NotConAllowable, "there is an invalid user-defined value"},

    // config and registry: file management errors
    {Errors::NotViaEOF, "didn't find an <ENDOFFILE> line"},
    {Errors::ProblematicFile, "there is a problematic file"},

    // config: writing
    {Errors::IllegalRoutingMode, "illegal routing mode"},
    {Errors::IllegalTagSubtagPair, "there is an illegal tag-subtag_variable pair"},

    // config: format
    {Errors::CoreSubtagMissing, "there is a core subtag in config for which no data has been entered"},
    {Errors::CoreTagMissing, "there is a core tag in config for which no data has been entered"},

    // registry: interconnection network/routing algorithm
    {Errors::BadlyFormedDimensionName,
    "there is a badly formatted dimension name for an interconnection network"},
    {Errors::EmptyDimensionName, "there is an empty dimension name for an interconnection network"},
    {Errors::EmptyIntNetName, "there is an empty interconnection network name"},
    {Errors::ExistingDimensionName, "there is a repeated dimension name for an interconnection network"},
    {Errors::NoRoutingAlgs, "there is no routing algorithm for an interconnection network"},
    {Errors::NotRegAllowable, "there is an invalid interconnection network parameter"},
    {Errors::RepeatedIntNetName, "there is a repeated interconnection network name"},
    {Errors::RepeatedRoutingAlg, "there is a repeated routing algorithm for an interconnection network"},
    {Errors::WrongNumParameters, "there are not four parameters for an interconnection network"},

    // registry: traffic pattern errors
    {Errors::EmptyTPname, "there is an empty traffic pattern name (so omitted)"},
    {Errors::InvalidTPname, "there is a repeated traffic pattern name (repetition omitted)"},

    // registry: notes
    {Errors::EmptyIntNetForNote, "there is no interconnection network for a note"},
    {Errors::InvalidNote, "there is an invalid interconnection network note"},

    // validation
    {Errors::AllFaults, "a fault description wants to make all components faulty and so has been ignored"},
    {Errors::AmalgamatedNotes, "some notes have been amalgamated"},
    {Errors::BadDimName, "there is a bad dimension name"},
    {Errors::BadExpandedDimName, "there is a bad expanded dimension name"},
    {Errors::FatalRegistry, "the registry has a fatal error"},
    {Errors::IllFormedtp, "there is bad formatting in a tp and so the default has been chosen"},
    {Errors::IntNetWithNoNote, "there is a registered interconnection network with no note"},
    {Errors::MismatchedParams,  "there is an error with the dimension names and values"},
    {Errors::NoIntNetsRegistered, "no interconnection networks have been registered"},
    {Errors::NoNoteForIntNet, "an interconnection network was registered without a note"},
    {Errors::NoTPsRegistered, "no traffic patterns have been registered"},
    {Errors::RegistryRewriteFail, "rewriting the registry failed"},
    {Errors::UnregNoteDeleted, "at least one unregistered note has been deleted"},
    {Errors::UnregRAName, "unregistered routing algorithm name"},
    {Errors::UnregTpName, "there is an unregistered traffic pattern name"},

    // coding errors
    {Errors::BadCoding, "there is a coding error"},
    {Errors::BadRoutingAlg, "routing algorithm does not exist"},
    {Errors::BadRoutingPrelude, "routing prelude does not exist"},
    {Errors::BadTPName, "traffic pattern does not exist"},
    {Errors::EmptyDelimiter, "there is a delimiter missing"},
    {Errors::ShortNameMismatch, "there is a short-name mismatch"},
    {Errors::TypeMismatch, "there is a type mismatch"},

    // user interconnection networks building
    {Errors::BadLengthTuple, "algebraic ids of the interconnection network have the wrong size"},
    {Errors::BuildingError,
    "there is some sort of user building error when building the interconnection network"},
    {Errors::ExecutionError, "there is some sort of execution error"},
    {Errors::DimensionTooLarge, "dimension is too large"},
    {Errors::OutOfRangeRelId, "a relative id is out of range"},
    {Errors::OutOfRangeTuple, "a component of an algebraic id is out of range"},

    // interconnection networks building
    {Errors::BadDestination, "the destination of a route is faulty"},
    {Errors::BadError, "the error flag of a route is faulty"},
    {Errors::FaultyChannel, "a channel of a route is faulty"},
    {Errors::FaultyNode, "a node of a route is faulty"},
    {Errors::FaultySource, "the source of a route is faulty"},
    {Errors::IllDefinedAlgRep, "the algebraic representation of the interconnection network is ill-defined"},
    {Errors::NoComponents, "there are no nodes or channels in the interconnection network"},

    // imposing faults
    {Errors::BadFaultNumber, "there is a bad number of faults"},
    {Errors::BadToken, "there is a bad fault token"},
    {Errors::EmptyFaultDesc, "there are zero components to make faulty"},
    {Errors::FaultNumberNotFoundYet, "still looking for a number of faults"},
    {Errors::MissingTag, "there is a missing tag in a fault file"},

    // file management
    {Errors::CantOpenFile, "a file could not be opened"},
    {Errors::NoFaultFolder, "there is no fault file folder"},
    {Errors::NoIntNetFolder, "there is no interconnection network folder"},

    // system
    {Errors::OutOfRangeNodeType, "a variable holding a node type is out of range"}
};

/**
**************
error messages
**************
**/

string format_error(// supplied data:
                    const string&                  type,
                    const string&                  msg,
                    const string&                  context,
                    const std::source_location&    loc)
/*
Write a formatted error message.
*/
{
    string the_string = "\n*** "s + type + "\n";
    the_string += "- "s + string(loc.file_name()) + " @ line " + std::to_string(loc.line()) + "\n";
    the_string += "- ("s + std::string(loc.function_name()) + ")\n";
    the_string += "- "s + msg;
    if (!context.empty())
        the_string += " ["s + context + "]";
    the_string += "\n- "s;
    return the_string;
}

void fatal(// supplied data:
            Errors                         the_error,
            const string&                  context,
            const std::source_location&    loc)
/*
Write a fatal error and then terminate.
*/
{
    const string &msg = error_messages.at(the_error);
    std::cerr << format_error("FATAL ERROR", msg, context, loc);
    throw std::runtime_error(msg);
}

void coding(// supplied data:
            Errors                         the_error,
            const string&                  context,
            const std::source_location&    loc)
/*
Write a coding error and then terminate.
*/
{
    const string &msg = error_messages.at(the_error);
    std::cerr << format_error("CODING ERROR", msg, context, loc);
    throw std::runtime_error(msg);
}

void warning(// supplied data:
            Errors                         the_error,
            const string&                  context,
            const std::source_location&    loc)
/*
Write a warning error.
*/
{
    const string &msg = error_messages.at(the_error);
    std::cerr << format_error("warning", msg, context, loc);
}

string short_warning(// supplied data:
                     Errors           the_error,
                     const string&    context)
/*
Return a short warning message string.
*/
{
    const string msg = error_messages.at(the_error);
    string the_warning = " *** warning: " + msg;
    if (!context.empty())
        the_warning += " [" + context + "]";
    return the_warning;
}
