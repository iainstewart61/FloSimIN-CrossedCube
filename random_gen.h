
#ifndef RANDOM_GEN_H
#define RANDOM_GEN_H

#include <random>
#include <vector>
#include <chrono>
#include <cstddef>
#include <stdexcept>

#include "structs/structs_system.h"

/**
This cpp contains a class concerning random number generation. Seeds come in the form of tuples of
'System::num_seeds' = 8 unsigned integers of 32 bits, i.e., uint32_t. I only ever use the functions:
 - getSizeT()
 - get().
Each member of this class has:
 - its own seed data
 - its own std::mt19937 engine
so my 4 random number generators;
 - RNG_fault
 - RNG_tp
 - RNG_misc
 - RNG_routing
are all independent.
**/

class RandomGenerator {
public:
    using SeedType = std::seed_seq::result_type;

    // construct RNG with either custom seeds or auto seeds
    explicit RandomGenerator(const std::vector<SeedType>& seeds = {})
    {
        createGenerator(seeds);
    }

    // get the actual seeds used
    const std::vector<SeedType> &getUsedSeeds() const
    {
        return used_seeds;
    }

    // generate random int in [min, max]
    int getInt(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng_instance);
    }

    // generate random size_t in [min, max]
    std::size_t getSizeT(std::size_t min, std::size_t max)
    {
        std::uniform_int_distribution<std::size_t> dist(min, max);
        return dist(rng_instance);
    }

    // generate a std::mt19937
    std::mt19937 &get()
    {
        return rng_instance;
    }

private:
    void createGenerator(const std::vector<SeedType>& seeds)
    {
        std::vector<SeedType> final_seeds;

        if (!seeds.empty())
        {
            final_seeds = seeds;
        }
        else
        {
            std::random_device rd{};
            final_seeds = {static_cast<SeedType>(std::chrono::steady_clock::now().time_since_epoch().count()),
                rd(), rd(), rd(), rd(), rd(), rd(), rd()};
            if (final_seeds.size() != System::num_seeds)// check the correct number of seeds
                throw std::runtime_error("there is the incorrect number of seeds in the random number generator!");
        }

        used_seeds = final_seeds;

        std::seed_seq ss(final_seeds.begin(), final_seeds.end());
        rng_instance = std::mt19937(ss);
    }

    // instance state (no longer static!)
    std::vector<SeedType> used_seeds;
    std::mt19937 rng_instance;
};

#endif
