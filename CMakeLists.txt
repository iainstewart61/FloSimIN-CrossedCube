cmake_minimum_required(VERSION 3.16)

project(FloSimINPaper VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(FloSimINPaper

main.cpp
execution.cpp
globals.cpp
maps.cpp
node.cpp
port.cpp

construction/int_net_construction.cpp

data_logging/data_logging.cpp

errors/error_writing.cpp

faults/impose_faults_from_description.cpp
faults/read_faults_from_file.cpp
faults/write_faults_to_file.cpp

input/data_structures_for_config.cpp
input/read_config_file.cpp
input/read_registry_file.cpp
input/validation.cpp
input/writing_config_reg.cpp

misc/misc_formatting_functions.cpp
misc/misc_string_functions.cpp

output/writing_output.cpp

statistics/collect_and_compute_stats.cpp
statistics/core_cumulatives.cpp

traffic/traffic_generators.cpp

ACTUAL_NETWORKS/hypercube/hypercube.cpp
ACTUAL_NETWORKS/hypercube/hypercube_routing.cpp

ACTUAL_NETWORKS/crossedcube/crossedcube.cpp
ACTUAL_NETWORKS/crossedcube/crossedcube_routing.cpp

)

target_include_directories(FloSimINPaper PRIVATE

${CMAKE_CURRENT_SOURCE_DIR}

construction
data_logging
errors
faults
input
misc
output
statistics
structs
traffic

ACTUAL_NETWORKS/hypercube
ACTUAL_NETWORKS/crossedcube

)

# ============================================================
# COPY DATA FILES (copy folder, then remove .cpp and .h)
# ============================================================

file(COPY ${CMAKE_SOURCE_DIR}/ACTUAL_NETWORKS
     DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE ACTUAL_CPP
    ${CMAKE_BINARY_DIR}/ACTUAL_NETWORKS/*.cpp
)

file(GLOB_RECURSE ACTUAL_HEADERS
    ${CMAKE_BINARY_DIR}/ACTUAL_NETWORKS/*.h
)

file(REMOVE ${ACTUAL_CPP})
file(REMOVE ${ACTUAL_HEADERS})

configure_file(${CMAKE_SOURCE_DIR}/config.txt
               ${CMAKE_BINARY_DIR}/config.txt
               COPYONLY)

configure_file(${CMAKE_SOURCE_DIR}/registry.txt
               ${CMAKE_BINARY_DIR}/registry.txt
               COPYONLY)

# ============================================================

if(MSVC)
    target_compile_options(FloSimINPaper PRIVATE /W4)
else()
    target_compile_options(FloSimINPaper PRIVATE -Wall -Wextra)
endif()