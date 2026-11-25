
#ifndef ERROR_WRITING_H
#define ERROR_WRITING_H

#include <string>
#include <vector>
#include <cstddef>
#include <source_location>
#include <unordered_map>

enum class Errors : std::size_t
/*
An enum of all possible errors.
*/
{
    // no error
    Good,
    // config and registry: string manipulation
    EmptyString, MissingDelimiter, MissingLeftBookend, MissingRightBookend, NotValidBool, NotValidDouble,
    NotValidInt, NotValidSize_t,
    // config and registry: list manipulation
    BadLengthList, EmptyListOfSize_t, EmptyStringInList, NotValidListOfSize_t, NotValidListOfUint32_t,
    NotValidNamedListOfSize_t,
    // config and registry: out of range values
    OutOfRangeDouble, OutOfRangeInt, OutOfRangeListOfSize_t, OutOfRangeListOfUint32_t, OutOfRangeSize_t,
    // config: traffic pattern errors
    BadPrefixedAlgId, BadPrefixedRelId, BadTpPercent, InvalidtpDesc, NoSourcesDests,
    // config: fault errors
    IllFormedNumberFault, InvalidFaultDesc, NumberFaultOutOfBounds,
    // config: user-defined values errors
    NotConAllowable,
    // config and registry: file management errors
    NotViaEOF, ProblematicFile,
    // config: writing
    IllegalRoutingMode, IllegalTagSubtagPair,
    // config: format
    CoreSubtagMissing, CoreTagMissing,
    // registry: interconnection network/routing algorithm
    BadlyFormedDimensionName, EmptyDimensionName, EmptyIntNetName, ExistingDimensionName, NoRoutingAlgs,
    NotRegAllowable, RepeatedIntNetName, RepeatedRoutingAlg, WrongNumParameters,
    // registry: traffic pattern errors
    EmptyTPname, InvalidTPname,
    // registry: notes
    EmptyIntNetForNote, InvalidNote,
    // validation
    AllFaults, AmalgamatedNotes, BadDimName, BadExpandedDimName, FatalRegistry, IllFormedtp,
    IntNetWithNoNote, MismatchedParams, NoIntNetsRegistered, NoNoteForIntNet, NoTPsRegistered,
    RegistryRewriteFail, UnregIntNetName, UnregNoteDeleted, UnregRAName, UnregTpName,
    // coding errors
    BadCoding, BadRoutingAlg, BadRoutingPrelude, BadTPName, EmptyDelimiter, ShortNameMismatch, TypeMismatch,
    // user interconnection networks building
    BadLengthTuple, BuildingError, ExecutionError, DimensionTooLarge, OutOfRangeRelId, OutOfRangeTuple,
    // interconnection networks building
    BadDestination, BadError, FaultyChannel, FaultyNode, FaultySource, IllDefinedAlgRep,
    NoComponents,
    // imposing faults
    BadFaultNumber, BadToken, EmptyFaultDesc, FaultNumberNotFoundYet, MissingTag,
    // file management
    CantOpenFile, NoFaultFolder, NoIntNetFolder,
    // system
    OutOfRangeNodeType
};

extern const std::unordered_map<Errors, std::string> error_messages;

/*
**************
error messages
**************
*/

void fatal(// supplied data:
           Errors                         the_error,
           const std::string&             context = "",
           const std::source_location&    loc = std::source_location::current());

void coding(// supplied data:
            Errors                         the_error,
            const std::string&             context = "",
            const std::source_location&    loc = std::source_location::current());

void warning(// supplied data:
             Errors                         the_error,
             const std::string&             context = "",
             const std::source_location&    loc = std::source_location::current());

std::string short_warning(// supplied data:
                          Errors                the_error,
                          const std::string&    context = "");

#endif
