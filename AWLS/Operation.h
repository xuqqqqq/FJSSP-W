/**
 * @file Operation.h
 * @brief Defines operation structures for FJSP and FJSSP-W
 */

#ifndef FJSP_OPERATION_H
#define FJSP_OPERATION_H

#include <algorithm>
#include <climits>
#include <unordered_map>
#include <vector>
#include "Instance.h"

struct Operation
{
    int id;
    int job_id;
    int index;
    std::vector<int> candidates;
    int w = INT_MAX;
    int t = 0;
    double z = 0;

#ifdef MAP_MACHINE_DURATION
    std::unordered_map<int, int> duration;
#else
    std::vector<int> duration;
#endif

    // Worker-level durations for FJSSP-W.
    // worker_durations[machine][worker] = processing time, -1 means infeasible.
    std::vector<std::vector<int>> worker_durations;
    std::vector<std::vector<int>> worker_candidates;
    std::vector<int> best_worker;

#ifndef MAP_MACHINE_DURATION
    constexpr
#endif
    Operation(int id, int job_id, int index) noexcept : id(id), job_id(job_id), index(index)
    {
    }

    [[nodiscard]] int operator[](int machine_id) const
    {
#ifdef MAP_MACHINE_DURATION
        auto it = duration.find(machine_id);
        return it != duration.end() ? it->second : -1;
#else
        if (machine_id < 0 || machine_id >= static_cast<int>(duration.size()))
            return -1;
        return duration[machine_id];
#endif
    }

    [[nodiscard]] bool canBeProcessedOn(int machine_id) const
    {
#ifdef MAP_MACHINE_DURATION
        return duration.contains(machine_id);
#else
        return machine_id < static_cast<int>(duration.size()) && duration[machine_id] >= 0;
#endif
    }

    [[nodiscard]] int workerDuration(int machine_id, int worker_id) const
    {
        if (machine_id < 0 || machine_id >= static_cast<int>(worker_durations.size()))
            return -1;
        if (worker_id < 0 || worker_id >= static_cast<int>(worker_durations[machine_id].size()))
            return -1;
        return worker_durations[machine_id][worker_id];
    }

    [[nodiscard]] const std::vector<int>& workersForMachine(int machine_id) const
    {
        static const std::vector<int> empty;
        if (machine_id < 0 || machine_id >= static_cast<int>(worker_candidates.size()))
            return empty;
        return worker_candidates[machine_id];
    }

    [[nodiscard]] int bestWorkerForMachine(int machine_id) const
    {
        if (machine_id < 0 || machine_id >= static_cast<int>(best_worker.size()))
            return -1;
        return best_worker[machine_id];
    }
};

struct OperationList
{
    std::vector<Operation> operations;
    bool has_worker_flexibility{false};
    int worker_count{1};

    OperationList() = default;

    explicit OperationList(const Instance& instance);

    Operation& operator[](int operation_id) { return operations[operation_id]; }
    const Operation& operator[](int operation_id) const { return operations[operation_id]; }

    [[nodiscard]] int duration(int operation_id, int machine_id) const
    {
        if (operation_id <= 0 || operation_id >= static_cast<int>(operations.size()) - 1)
            return 0;
        if (machine_id < 0 || machine_id >= static_cast<int>(operations[operation_id].duration.size()))
            return 0;

        return operations[operation_id][machine_id];
    }

    [[nodiscard]] int worker_duration(int operation_id, int machine_id, int worker_id) const
    {
        if (operation_id <= 0 || operation_id >= static_cast<int>(operations.size()) - 1)
            return 0;
        return operations[operation_id].workerDuration(machine_id, worker_id);
    }

    [[nodiscard]] const std::vector<int>& workers_for_machine(int operation_id, int machine_id) const
    {
        static const std::vector<int> empty;
        if (operation_id <= 0 || operation_id >= static_cast<int>(operations.size()) - 1)
            return empty;
        return operations[operation_id].workersForMachine(machine_id);
    }

    [[nodiscard]] int best_worker_for_machine(int operation_id, int machine_id) const
    {
        if (operation_id <= 0 || operation_id >= static_cast<int>(operations.size()) - 1)
            return -1;
        return operations[operation_id].bestWorkerForMachine(machine_id);
    }

    [[nodiscard]] bool has_workers() const { return has_worker_flexibility; }
    [[nodiscard]] int worker_num() const { return worker_count; }

    [[nodiscard]] size_t size() const { return operations.size(); }
    [[nodiscard]] size_t realOperationCount() const { return operations.size() - 2; }

    auto begin() { return operations.begin(); }
    [[nodiscard]] auto begin() const { return operations.begin(); }
    auto end() { return operations.end(); }
    [[nodiscard]] auto end() const { return operations.end(); }
};

inline OperationList::OperationList(const Instance& instance)
{
    has_worker_flexibility = instance.has_worker_flexibility;
    worker_count = std::max(1, instance.worker_num);

    const int total_operations = instance.op_num + 2;
    operations.reserve(total_operations);

    operations.emplace_back(0, -1, -1);

    int id = 1;
    for (int job_id = 0; job_id < instance.job_num; ++job_id)
    {
        for (int operation_idx = 0; operation_idx < static_cast<int>(instance.jobs[job_id].size()); ++operation_idx)
        {
            Operation operation{ id++, job_id, operation_idx };

#ifndef MAP_MACHINE_DURATION
            operation.duration.resize(instance.machine_num, -1);
#endif
            operation.worker_durations.assign(instance.machine_num, std::vector<int>(worker_count, -1));
            operation.worker_candidates.resize(instance.machine_num);
            operation.best_worker.assign(instance.machine_num, -1);

            const auto& op_info = instance.jobs[job_id][operation_idx];
            operation.candidates.reserve(op_info.size());

            for (const auto& candidate : op_info)
            {
                const int machine = candidate.machine;
                operation.candidates.push_back(machine);
                operation.duration[machine] = candidate.duration;

                int best_worker = -1;
                int best_duration = INT_MAX;
                for (const auto& worker_option : candidate.worker_options)
                {
                    if (worker_option.worker < 0 || worker_option.worker >= worker_count)
                    {
                        continue;
                    }
                    operation.worker_durations[machine][worker_option.worker] = worker_option.duration;
                    operation.worker_candidates[machine].push_back(worker_option.worker);
                    if (worker_option.duration < best_duration)
                    {
                        best_duration = worker_option.duration;
                        best_worker = worker_option.worker;
                    }
                }

                if (best_worker == -1)
                {
                    operation.worker_durations[machine][0] = candidate.duration;
                    operation.worker_candidates[machine].push_back(0);
                    best_worker = 0;
                    best_duration = candidate.duration;
                }

                operation.best_worker[machine] = best_worker;
                operation.duration[machine] = best_duration;
            }

            operations.push_back(std::move(operation));
        }
    }

    operations.emplace_back(id, -1, -1);
}

#endif // FJSP_OPERATION_H
