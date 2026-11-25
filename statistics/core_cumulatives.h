
#ifndef CORE_CUMULATIVES_H
#define CORE_CUMULATIVES_H

#include <vector>
#include <cstddef>

std::size_t compute_cumulative_number(// supplied data:
                                      const std::vector<std::size_t>&    cum_v);

std::size_t compute_cumulative_sum(// supplied data:
                                   const std::vector<std::size_t>&    cum_v);

double compute_cumulative_average(// supplied data:
                                  const std::size_t    sum,
                                  const std::size_t    number);
double compute_cumulative_std_dev(// supplied data:
                                  const std::vector<std::size_t>&    cum_v,
                                  const double                       average,
                                  const std::size_t                  number);

double compute_cumulative_coeff_var(// supplied data:
                                    const double    average,
                                    const double    std_dev);

double compute_cumulative_median(// supplied data:
                                 const std::vector<std::size_t>&    cum_v,
                                 const std::size_t                  number);

std::size_t compute_cumulative_minimum(// supplied data:
                                       const std::vector<std::size_t>&    cum_v);

std::size_t compute_cumulative_maximum(// supplied data:
                                       const std::vector<std::size_t>&    cum_v);

#endif
