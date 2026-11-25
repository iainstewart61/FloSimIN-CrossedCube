
#include <numeric>
#include <string>
#include <cmath>

#include "core_cumulatives.h"

using std::vector;
using std::string;
using std::size_t;

/**
This cpp contains basic functions that perform cumulative statistical computations.

Functions:
When applied, the assumption is that the following functions will be executed in order:
 - compute_cumulative_number()
 - compute_cumulative_sum()
 - compute_cumulative_average()   (needs number)
 - compute_cumulative_std_dev()   (needs average, number)
 - compute_cumulative_coeff_var() (needs average, std_dev)
 - compute_cumulative_median()    (needs number)
 - compute_cumulative_minimum()
 - compute_cumulative_mmaximum()
so as to save on recomputations.
**/

size_t compute_cumulative_number(// supplied data:
                                 const vector<size_t>&    cum_v)
/*
Compute the number of items in a cumulative vector [n_0, n_1, ..., n_T], i.e.,
 - Sigma(n_i)
If 'cum_v' is empty then 0 is returned.
*/
{
    size_t number = accumulate(cum_v.begin(), cum_v.end(), size_t(0));
    return number;
}

size_t compute_cumulative_sum(// supplied data:
                              const vector<size_t>&    cum_v)
/*
Compute the sum from a cumulative vector [n_0, n_1, ..., n_T], i.e.,
 - (n_0 * 0) + (n_1 * 1) + ... + (n_T * T).
If 'cum_v' is empty then 0 is returned.
*/
{
    size_t length = cum_v.size();
    size_t sum = 0;
    for (size_t i = 0; i < length; ++i)
    {
        sum += i * cum_v[i];
    }
    return sum;
}

double compute_cumulative_average(// supplied data:
                                  const size_t    sum,
                                  const size_t    number)
/*
Compute the average 'sum' / 'number'. If 'number' = 0 then 0.0 is returned.
*/
{
    if (number == 0)
        return 0.0;
    else
        return static_cast<double>(sum) / number;
}

double compute_cumulative_std_dev(// supplied data:
                                  const vector<size_t>&    cum_v,
                                  const double             average,
                                  const size_t             number)
/*
Compute the standard deviation from a cumulative vector [n_0, n_1, ..., n_T] w.r.t. 'average' and
where 'number' = Sigma(n_i), i.e.,
 - sq_rt[Sigma n_i(i - average)^2 / number]
If 'cum_v' is empty or 'number' = 0 then 0.0 is returned.
*/
{
    if (number == 0)
        return 0.0;
    size_t length = cum_v.size();
    double sum_sq = 0.0;
    double diff;
    for (size_t i = 0; i < length; ++i)
    {
        diff = static_cast<double>(i) - average;
        sum_sq += cum_v[i] * diff * diff;
    }
    return std::sqrt(sum_sq / number);
}

double compute_cumulative_coeff_var(// supplied data:
                                    const double    average,
                                    const double    std_dev)
/*
Compute the coefficient of variation from a cumulative vector [n_0, n_1, ..., n_T], i.e.,
 - 'std_dev' / 'average'
If 'average' = 0 then 0.0 is returned.
*/
{
    if (average > 0)
        return std_dev / average;
    else
        return 0.0;
}

double compute_cumulative_median(// supplied data:
                                 const vector<size_t>&    cum_v,
                                 const size_t             number)
/*
Compute the median from a cumulative vector [n_0, n_1, ..., n_T] where 'number' = Sigma(n_i), i.e.,
 - 0, ..., 0 (n_0 times), 1, ..., 1 (n_1 times), ... mid-point ..., T, ..., T (n_T times)
If 'cum_v' is empty then 0.0 is returned.
*/
{
    size_t length = cum_v.size();
    if ((length == 0) || (number == 0))
        return 0.0;
    size_t mid_pt;
    if (number % 2 == 1)
        mid_pt = (number + 1) / 2;      // odd number of items (easy)
    else
        mid_pt = (number / 2) + 1;      // even number of items (harder: 'mid_pt' = right-middle-item)
    size_t running_sum = 0;             // sum of cum_v[0] + ... + cum_v[i] in ith iteration
    size_t prev_running_sum = 0;        // sum of cum_v[0] + ... + cum_v[i - 1] in (i - 1)th iteration
    for (size_t i = 0; i < length; ++i)
    {
        running_sum += cum_v[i];
        if (mid_pt <= running_sum)
        {
            if (number % 2 == 1)
                return static_cast<double>(i);
            else
            {
                if (mid_pt - 1 > prev_running_sum)
                    return static_cast<double>(i);
                else
                    return (static_cast<double>(i) + (i - 1)) / 2;
            }
        }
        prev_running_sum = running_sum;
    }
    return 0.0;                                 // we never get to here!
}

size_t compute_cumulative_minimum(// supplied data:
                                  const vector<size_t>&    cum_v)
/*
Compute the minimum from a cumulative vector [n_0, n_1, ..., n_T], i.e.,
 - least i for which n_i > 0
If 'cum_v' is empty or all components are 0 then return 0.
*/
{
    size_t length = cum_v.size();
    for (size_t i = 0; i < length; ++i)
    {
        if (cum_v[i] > 0)
            return i;
    }
    return 0;
}

size_t compute_cumulative_maximum(// supplied data:
                                  const vector<size_t>&    cum_v)
/*
Compute the maximum from a cumulative vector [n_0, n_1, ..., n_T], i.e.,
 - greatest i for which n_i > 0
If 'cum_v' is empty or all components are 0 then return 0.
*/
{
    size_t length = cum_v.size();
    for (size_t i = length; i-- > 0; )
    {
        if (cum_v[i] > 0)
            return i;
    }
    return 0;
}
