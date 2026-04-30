//
// Created by qiming on 25-4-13.
//

#ifndef FJSP_TABUSEARCH_H
#define FJSP_TABUSEARCH_H
#include <algorithm>
#include <atomic>
#include <cstddef>
#include "NeighborhoodMove.h"
#include "Schedule.h"
#include "TabuList.h"

enum class CriticalBlockResource
{
    Machine,
    Worker
};

struct CriticalBlock
{
    CriticalBlockResource resource;
    int resource_id;
    std::vector<int> operations;
};

class TabuSearch
{
public:
    [[nodiscard]] int get_makespan() const { return best_schedule.makespan; }
    [[maybe_unused]] [[nodiscard]] unsigned long long get_iteration() const { return iteration; }
    static constexpr int gamma = 40, beta = 500, theta = 5;

    explicit TabuSearch(const Instance& instance) :
        tabu_list(instance.machine_num), L(10 + instance.job_num / instance.machine_num),
        L_max(instance.job_num <= 2 * instance.machine_num ? static_cast<int>(L * 1.4) : static_cast<int>(L * 1.5))
    {
        int candidate_count = 0;
        for (const auto& job : instance.jobs)
        {
            for (const auto& operation : job)
            {
                candidate_count += static_cast<int>(operation.size());
            }
        }
        const double relative_machine_flexibility = static_cast<double>(candidate_count) /
            std::max(1, instance.op_num * instance.machine_num);
        const bool tighten_worker_probe_shortlists =
            instance.has_worker_flexibility && instance.op_num >= 250 && relative_machine_flexibility >= 0.45;
        worker_change_shortlist_size = tighten_worker_probe_shortlists ? 1u : 2u;
        worker_change_strong_shortlist_size = tighten_worker_probe_shortlists ? 2u : 3u;
    }

    void search(const Schedule& schedule, const std::atomic<bool>& stop_flag);

    [[nodiscard]] NeighborhoodMove find_move(const std::atomic<bool>* stop_flag = nullptr);
    void make_move(const NeighborhoodMove& move);

    Schedule best_schedule{};


private:
    unsigned long long iteration{};
    Schedule current_schedule{};
    TabuList tabu_list;
    int L;
    int L_max;
    size_t worker_change_shortlist_size{2};
    size_t worker_change_strong_shortlist_size{3};

    void change_machine_evaluate_and_push(const Schedule& schedule, const NeighborhoodMove& move,
        const std::vector<int>& intersection,
        std::vector<NeighborhoodMove>& all_moves,
        std::vector<NeighborhoodMove>& best_moves, int& min_makespan) const;

    std::vector<int> critical_path;
    std::vector<CriticalBlock> critical_blocks;

    [[nodiscard]] bool is_tabu(const NeighborhoodMove& move, int makespan) const;
    void update_critical_block();
    void update_all_critical_block();
    void same_machine_evaluate_and_push(const Schedule& schedule, const NeighborhoodMove& move,
        std::vector<NeighborhoodMove>& all_moves,
        std::vector<NeighborhoodMove>& best_moves, int& min_makespan) const;
};

#endif // FJSP_TABUSEARCH_H

