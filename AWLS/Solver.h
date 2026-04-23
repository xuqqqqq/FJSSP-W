#ifndef MAE_H
#define MAE_H

#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#ifdef USE_OPENMP
#include <omp.h>
#endif
#include "RandomGenerator.h"
#include "Schedule.h"
#include "TabuSearch.h"

class Solver
{
    static std::atomic<bool> stop_flag;

    // ¸¨Öúº¯Êý
    static void run_timer_thread(long long time_limit);
    static void initialize_population(const Instance& instance, std::shared_ptr<OperationList>& operation_list,
        std::vector<Schedule>& population);
    static void update_best_solution(const std::vector<Schedule>& population, Schedule& best_solution, int best);
    static void population_maintenance(std::vector<Schedule>& population, const Instance& instance,
        std::shared_ptr<OperationList>& operation_list, int gen);

public:
    enum SolverMode
    {
        SERIAL,
        PARALLEL
    };
#ifdef USE_OPENMP
    static void Solve(const Instance& instance, long long time_limit, int best, SolverMode mode = PARALLEL);
#else
    static void Solve(const Instance& instance, long long time_limit, int best, SolverMode mode = SERIAL);
#endif
};

#endif // MAE_H
