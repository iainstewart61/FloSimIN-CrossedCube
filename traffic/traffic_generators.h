
#ifndef TRAFFIC_GENERATORS_H
#define TRAFFIC_GENERATORS_H

#include <iostream>
#include <vector>
#include <cstddef>
#include <utility>

bool random_traffic_generator(// supplied data:
                              const std::size_t                                     tp_number,
                              //supplied/computed data:
                              bool&                                                 started,
                              std::size_t&                                          source,
                              std::size_t&                                          dest,
             [[maybe_unused]] std::vector<std::pair<std::size_t, std::size_t>> &    precomputed_tp,
             [[maybe_unused]] std::size_t&                                          index);

bool alltoall_traffic_generator(// supplied data:
                                const std::size_t                                     tp_number,
                                //supplied/computed data:
                                bool&                                                 started,
                                std::size_t&                                          source,
                                std::size_t&                                          dest,
               [[maybe_unused]] std::vector<std::pair<std::size_t, std::size_t>> &    precomputed_tp,
               [[maybe_unused]] std::size_t&                                          index);

bool permutation_traffic_generator(// supplied data:
                                   const std::size_t                                    tp_number,
                                   //supplied/computed data:
                                   bool&                                                started,
                                   std::size_t&                                         source,
                                   std::size_t&                                         dest,
                                   std::vector<std::pair<std::size_t, std::size_t>>&    precomputed_tp,
                                   std::size_t&                                         index);

#endif
