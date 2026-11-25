## Minimal routing in crossed cubes

FloSimIN is an in-house simulator for the study of flow-based communication in interconnection networks with regard to routing algorithms, traffic patterns and imposed faults (it is currently under construction and will be released in full at a later date). This repository contains the source code, configuration files, fault descriptions and documentation required to reproduce the experiments presented in the following submission:

- Iain A. Stewart, "Minimal routing in crossed cubes" submitted to IEEE Transactions on Parallel and Distributed Systems.

Abstract: We present a new minimal routing algorithm for the well-established interconnection networks known as crossed cubes. We prove that this routing algorithm improves upon all existing minimal routing algorithms for crossed cubes and is, in a sense, definitive. We undertake an experimental evaluation of the new routing algorithm in the presence of faulty components and in comparison with existing minimal routing algorithms. We show that the performance of our routing algorithm eclipses that of existing minimal routing algorithms no matter whether the faults are node faults or channel faults and irrespective of the ratio of faulty to healthy components.

The author is Iain A. Stewart, Department of Computer Science, Durham University, Mathematical Sciences & Computer Science Building, Upper Mountjoy Campus, Stockton Road, Durham DH1 3LE, UK and this software is supplied under an MIT License.

## Requirements

**Directory structure**

The simulator accesses code and data stored via the directory structure:

FloSimINPaper  
│  
├── ACTUAL_NETWORKS  
│       ├── crossedcube  
│                ├── fault_files  
│                └── output_files  
│       ├── hypercube  
│                ├── fault_files  
│                └── output_files  
├── construction  
├── data_logging  
├── errors  
├── faults  
├── input  
├── misc  
├── output  
├── statistics  
├── structs  
└── traffic

The following .cpp and .h files sit within the project root, i.e., the directory FloSimINPaper:
- execution.cpp and execution.h
- globals.cpp and globals.h
- main.cpp and main.h
- maps.cpp and maps.h
- node.cpp and node.h
- port.cpp and port.h
- random_gen.h

The simulator also expects the following configuration files to be present in the project root:
- config.txt
- registry.txt

In addition, the following files also appear in the project root:
- CMakeLists.txt
- README.md

**Compiler**

The code was developed using Code::Blocks on Windows with the GNU GCC Compiler that is C++20-compliant. As noted above, all code sits within (directories within) the directory FloSimINPaper. Alternatively, it can be built on Windows via the following command line instructions from the project root:
- mkdir build
- cd build
- cmake ..
- cmake --build .

This produces the executable:
- build/FloSimINPaper.exe

**Running**

From the build directory, type:
- FloSimINPaper.exe

The output file from an execution will appear in the directory:
- build/ACTUAL_NETWORKS/crossedcube/output_files

## Experiments and configurations

The directories:
- build/ACTUAL_NETWORKS/crossedcube/fault_files
- build/ACTUAL_NETWORKS/crossedcube/output_files

hold, respectively: 
- folders of ready-made fault-files that specify collections of faults to be imposed in an interconnection network (a fault can be a failed node or a failed channel)
- folders of output-files from experiments (using the fault-files) undertaken as described in the paper. 

An output-file contains all parameter settings and statistics resulting from the experiment such as, for example, the percentage of failed routes, the average path-length of successful routes, the average load on a healthy channel, etc. By consulting the parameter settings in the output-files, all of the experiments from the paper can be reproduced (as described below).

**Experiment reproduction**

Experiment reproduction (along with any new experiments that might be chosen by the user to be undertaken) is done by setting experiment-specific parameters via the configuration file "config.txt" in the directory build. These parameters concern: the interconnection network name; the network size; the routing algorithm; the mode of the routing algorithm; the traffic pattern; the faults generated or imposed; how fault-checking is undertaken, etc. (again, as described below). The file "registry.txt" should not be touched (it is used to validate configuration settings). So, to reproduce an experiment:
- build the executable FloSimINPaper.exe (as above)
- choose the corresponding output-file (from build/ACTUAL_NETWORKS/crossedcube/output_files)
- set the parameters accordingly in "config.txt" in the directory build (see below)
- ensure that the relevant fault-files are present (in build/ACTUAL_NETWORKS/crossedcube/fault_files)
- execute the simulator from build (as above)
- obtain the new output-file (which appears in build/ACTUAL_NETWORKS/crossedcube/output_files)
- compare the statistics generated with those in the original output-file.

## Setting parameters in "config.txt"

In "config.txt", each instruction occupies one line and blank lines and spaces are ignored. There are tags after which come instructions pertinent to that tag. The final tag must be **\<END OF FILE\>**. 

After the tag **\<INTERCONNECTION NETWORK\>**, the only parameter that should be changed is the dimension "n" of the crossed cube via, e.g.,

- params = [n = 10]

This line must appear in "config.txt". The parameter "name" should remain set at *crossedcube*.

After the tag **\<ROUTING ALGORITHM\>**, there are three parameters that can be changed. The first is the parameter "name" denoting the name of the routing algorithm to be used in the simulation. This can be changed via, e.g.,

- name = crossedcube_Stewart

but the name of the routing algorithm must be supplied. There are three possibilities: *crossedcube_Efe*; *crossedcube_ChangSungHsu*; and *crossedcube_Stewart*.

The second parameter that can be changed is "mode" that denotes the mode of execution of the routing algorithm. All three routing algorithms are non-deterministic and the mode can be set via, e.g.,

- mode = nondet

There are two possibilities: *nondet*; and *full*. When the mode is *nondet*, the routing algorithm executes simply by non-deterministically making guesses (when a guess has to be made). When the mode is *full*, there is an execution for *every* possible sequence of guesses, assuming that routes fail, i.e., contain a faulty node or channel. If a healthy route is found then we end the execution (without trying as yet untried routes). Also, if the mode is *nondet* then we can set an upper bound "nondet_repeat" on the number of tries we should make to find a healthy route, in the event of faulty routes being found, via, e.g.,

- nondet_repeat = 10

If the parameter "nondet_repeat" is not supplied then the default value is 1. If the mode is *full* then the value of the parameter "nondet_repeat" has no impact.

After the tag **\<TRAFFIC PATTERN\>**, traffic patterns are defined. In order to set an all-to-all traffic pattern, we include the line

- [alltoall]

If we wish to run our chosen routing algorithm 3 times, say, on an all-to-all traffic pattern, one run after the other and so as to collect statistics for each run as well as over all runs, then we include the line

- [alltoall, 1, 3]

In order to set 100 random traffic patterns, say, then we include the line

- [random, 100]

If we wish to run our chosen routing algorithm 3 times, say, with each time undertaking a run of 100 random traffic patterns (so as to collect statistics for each run of 100 as well as over all runs) then we include the line

- [random, 100, 3]

The parameter "suppress_tps" after the tag **\<ENVIRONMENT\>** should remain untouched.

After the tag **\<FAULT\>**, we can impose faults from a fault description or read faults from a previously prepared fault-file. We can impose faults from a fault description by including the line, e.g.,

- [node]<25%>

so that 25% of the nodes are made faulty (via random choice). Including a line

- [channel]<25%>

similarly makes 25% of the channels faulty (where a channel is unidirectional from a node to a node). If we wish to make 25% of the links faulty then we include the line

- [link]<25%>

with a link consisting of a pair of oppositely oriented channels between two given nodes. If we omit the %-sign (in the above lines) then this makes exactly 25 nodes, channels or links faulty.

The parameter "fault_check" denotes whether routes found are actually checked for faults. If we wish to check for faults then we include the line

- fault_check = interim_at_end

and if we do not wish to check for faults then we either omit any line involving the parameter "fault_check" or include the line

- fault_check = none

If we wish to read faults from a previously prepared fault-file such as *fault_file_CC12_5pc_node_A.txt* then we include the line

- input_fault_files = fault_file_CC12_5pc_node_A

and if we wish to read faults from more than one previously prepared fault-file then we include a line such as 

- input_fault_files = [fault_file_CC12_10pc_node_A, fault_file_CC12_10pc_node_B, fault_file_CC12_10pc_node_C]

If there is more than one fault-file then the number of fault-files should match with the number that appears as the third parameter in [alltoall, 1, 3] or [random, 100, 3]. If this is the case: for the first run, the faults from *fault_file_CC12_10pc_node_A* are imposed; for the second run, the faults from *fault_file_CC12_10pc_node_B* are imposed; and for the third run, the faults from *fault_file_CC12_10pc_node_C* are imposed. As stated above, statistics are collected for each run and over all runs.

We can save the faults imposed by a fault description into a fault-file via the line

- output_fault_file = true

The fault-file generated will appear in the directory build/ACTUAL_NETWORKS/crossedcube/fault_files after execution has terminated (and can be used to impose faults in future runs).

Finally, for each of the tags **\<INTERCONNECTION_NETWORK\>**, **\<ROUTING_ALGORITHM\>**, **\<TRAFFIC_PATTERN\>** and **\<FAULT\>**, there is a parameter "seeds" which consists of a list of 8 natural numbers such as 

- seeds = [3491316736,779933330,3694568317,3111924482,676205162,2316012122,3869326078,1515352789]

and which gives the seeds for a random number generator associated with the tag. These seeds can be found in the output-files and should be included on a line if the experiment associated with that output-file is to be reproduced.

There are more parameters and additional functionality of our simulation code but the above suffices to reproduce the output-files supplied (and run some additional user-chosen experiments if required). Also, any text in "config.txt" appearing after the symbol # is treated as a comment. The current contents of "config.txt" (including the seeds) are such that the output-file *crossedcube_ChangSungHsu[n=12,alltoall,nondet1]-05pc_chan_collected.txt* in build/ACTUAL_NETWORKS/crossedcube/crossedcube_ChangSungHsu can be obtained.

## Example

An output-file header supplies all of the pertinent information with regard to the particular run and there is more detailed information at the end of the output-file after the results have been presented. Note that there is a timing on the first line of an output-file which gives the time taken for the experiment to run when executed on an HP laptop with processor AMD Ryzen 5 Pro 7535U with Radeon Graphics (2.90 GHz). 

The name of an output-file, e.g.,

- crossedcube_Efe[n=12,alltoall,nondet1]-5pc_node_collected.txt

denotes that the corresponding experiment: ran the routing algorithm *crossedcube_Efe* in a crossed cube of dimension 12 on an all-to-all traffic pattern where the mode was *nondet* with at most 1 attempt being made to find a healthy route and so that there was more than one run on this traffic pattern with 5% of nodes made faulty. More exact details of the experiment can be found inside the output-file.

If an output-file header consists of the following, as does the supplied output-file "crossedcube_Efe[n=12,alltoall,nondet1]-5pc_node_collected.txt":

date and time / elapsed time               : 03:03pm 10 Jun 2026 / 19 minutes 30 seconds

interconnection network                    : crossedcube [n=12]\
routing algorithm                          : crossedcube_Efe [mode=nondet1]\
notes                                      : -

fault seeds                                : [3491316736,779933330,3694568317,3111924482,676205162,2316012122,3869326078,1515352789]\
traffic pattern seeds                      : [3491293336,3386394681,2593613890,2440564673,619220207,2472142913,1325981796,534039402]\
routing seeds                              : [3491269336,4278619275,23983569,2400426910,4161080953,75261556,3818979398,951383548]\
miscellaneous seeds                        : [3491228036,2203791639,616662814,1676851387,3951401584,3436270038,284672457,1343844051]

fault descriptions:

input fault files:
 - input fault file #1                     : fault_file_CC12_5pc_node_A.txt
 - input fault file #2                     : fault_file_CC12_5pc_node_B.txt
 - input fault file #3                     : fault_file_CC12_5pc_node_C.txt
 - input fault file #4                     : fault_file_CC12_5pc_node_D.txt
 - input fault file #5                     : fault_file_CC12_5pc_node_E.txt

traffic patterns:
- traffic pattern #1                       : alltoall (1 iterations)\
  start-source                             : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-source                               : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]\
  start-destination                        : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-destination                          : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]
- traffic pattern #2                       : alltoall (1 iterations)\
  start-source                             : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-source                               : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]\
  start-destination                        : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-destination                          : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]
- traffic pattern #3                       : alltoall (1 iterations)\
  start-source                             : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-source                               : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]\
  start-destination                        : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-destination                          : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]
- traffic pattern #4                       : alltoall (1 iterations)\
  start-source                             : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-source                               : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]\
  start-destination                        : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-destination                          : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]
- traffic pattern #5                       : alltoall (1 iterations)\
  start-source                             : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-source                               : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]\
  start-destination                        : pr0    (   0) [0,0,0,0,0,0,0,0,0,0,0,0]\
  end-destination                          : pr4095 (4095) [1,1,1,1,1,1,1,1,1,1,1,1]

then "config.txt" should be set as the following if the experiment is to be reproduced exactly:

\<INTERCONNECTION NETWORK\>\
name = crossedcube\
params = [n = 12]\
seeds = [3491228036,2203791639,616662814,1676851387,3951401584,3436270038,284672457,1343844051]

\<ROUTING ALGORITHM\>\
name = crossedcube_Efe\
mode = nondet\
nondet_repeat = 1\
seeds = [3491269336,4278619275,23983569,2400426910,4161080953,75261556,3818979398,951383548]

\<TRAFFIC PATTERN\>\
[alltoall, 1, 5]\
seeds = [3491293336,3386394681,2593613890,2440564673,619220207,2472142913,1325981796,534039402]

\<ENVIRONMENT\>\
suppress_tps = false

\<FAULT\>\
fault_check = interim_at_end\
input_fault_files = [fault_file_CC12_5pc_node_A, fault_file_CC12_5pc_node_B, fault_file_CC12_5pc_node_C, fault_file_CC12_5pc_node_D, fault_file_CC12_5pc_node_E]\
seeds = [3491316736,779933330,3694568317,3111924482,676205162,2316012122,3869326078,1515352789]

\<END OF FILE\>







