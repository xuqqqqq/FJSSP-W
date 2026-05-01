//
// Created by qiming on 25-4-13.
//

#include "TabuSearch.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <set>
#include <vector>
#include "RandomGenerator.h"
#include "Schedule.h"
#include "DebugTrace.h"

namespace
{
constexpr bool kEnableAdaptiveWeightUpdate = false;
constexpr bool kEnableMiddleBlockMoves = false;
constexpr bool kEnableTwoBlockBackMove = false;
constexpr bool kEnableBackToFrontMoves = false;
constexpr bool kEnablePrefixFrontMoves = true;
constexpr bool kEnableFrontToBackMoves = false;
constexpr bool kEnableWorkerChangeNeighborhood = true;
constexpr bool kEnableChangeMachineNeighborhood = true;
constexpr unsigned long long kMaxTabuIterationsPerPass = 5000;

bool should_stop_search(const std::atomic<bool>* stop_flag)
{
    return stop_flag != nullptr && stop_flag->load(std::memory_order_relaxed);
}

std::vector<int> collect_worker_shortlist(const Schedule& schedule,
    const int op,
    const int machine,
    const int exclude_worker,
    const size_t max_candidates)
{
    std::vector<std::pair<int, int>> ranked_workers;
    const auto& operation_list = *schedule.get_operation_list_ptr();
    const auto& worker_candidates = operation_list.workers_for_machine(op, machine);
    ranked_workers.reserve(worker_candidates.size());

    for (const int worker : worker_candidates)
    {
        if (worker == exclude_worker)
        {
            continue;
        }

        const int duration = operation_list.worker_duration(op, machine, worker);
        if (duration <= 0)
        {
            continue;
        }
        ranked_workers.emplace_back(duration, worker);
    }

    std::sort(ranked_workers.begin(), ranked_workers.end(),
        [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
            if (lhs.first != rhs.first)
            {
                return lhs.first < rhs.first;
            }
            return lhs.second < rhs.second;
        });

    std::vector<int> shortlist;
    shortlist.reserve(std::min(max_candidates, ranked_workers.size()));
    for (const auto& [duration, worker] : ranked_workers)
    {
        (void)duration;
        shortlist.push_back(worker);
        if (shortlist.size() >= max_candidates)
        {
            break;
        }
    }

    return shortlist;
}

std::vector<int> collect_machine_shortlist(const Schedule& schedule,
    const int op,
    const int exclude_machine,
    const size_t max_candidates)
{
    const auto& operation_list = *schedule.get_operation_list_ptr();
    std::vector<std::pair<int, int>> ranked_machines;
    ranked_machines.reserve(operation_list[op].candidates.size());

    for (const int machine : operation_list[op].candidates)
    {
        if (machine == exclude_machine)
        {
            continue;
        }

        const int duration = operation_list.duration(op, machine);
        if (duration <= 0)
        {
            continue;
        }

        ranked_machines.emplace_back(duration, machine);
    }

    std::sort(ranked_machines.begin(), ranked_machines.end(),
        [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
            if (lhs.first != rhs.first)
            {
                return lhs.first < rhs.first;
            }
            return lhs.second < rhs.second;
        });

    std::vector<int> shortlist;
    const size_t limit = max_candidates == 0 ? ranked_machines.size() : std::min(max_candidates, ranked_machines.size());
    shortlist.reserve(limit);
    for (size_t i = 0; i < limit; ++i)
    {
        shortlist.push_back(ranked_machines[i].second);
    }

    return shortlist;
}

std::vector<NeighborhoodMove> collect_machine_insertion_shortlist(const Schedule& schedule,
    const int op,
    const int candidate_machine,
    const size_t max_candidates)
{
    const auto& graph = schedule.get_graph();
    const auto& time_info = schedule.get_time_info();
    const auto& operation_list = *schedule.get_operation_list_ptr();
    const int first_target_op = graph.first_machine_operation[candidate_machine];
    if (first_target_op == -1)
    {
        return {};
    }

    const int duration = operation_list.duration(op, candidate_machine);
    if (duration <= 0)
    {
        return {};
    }

    const int job_pred = graph.job_predecessor[op];
    const int job_ready_time = job_pred != -1 ? time_info[job_pred].end_time : 0;

    struct RankedInsertion
    {
        NeighborhoodMove move;
        int score;
        int overrun;
        int position;
    };

    std::vector<RankedInsertion> ranked_insertions;
    ranked_insertions.reserve(static_cast<size_t>(graph.machine_operation_count[candidate_machine]) + 1);

    auto push_candidate = [&](const Method method,
        const int where,
        const int predecessor_op,
        const int successor_op,
        const int position) {
            int start_time = job_ready_time;
            if (predecessor_op != -1)
            {
                start_time = std::max(start_time, time_info[predecessor_op].end_time);
            }

            const int completion_time = start_time + duration;
            int overrun = 0;
            if (successor_op != -1)
            {
                overrun = std::max(0, completion_time - time_info[successor_op].forward_path_length);
            }

            ranked_insertions.push_back({
                NeighborhoodMove{ method, op, where },
                completion_time + overrun,
                overrun,
                position
                });
        };

    push_candidate(Method::CHANGE_MACHINE_FRONT, first_target_op, -1, first_target_op, 0);

    int position = 1;
    for (int target_op = first_target_op; target_op != -1; target_op = graph.machine_successor[target_op], ++position)
    {
        push_candidate(Method::CHANGE_MACHINE_BACK,
            target_op,
            target_op,
            graph.machine_successor[target_op],
            position);
    }

    if (max_candidates == 0 || ranked_insertions.size() <= max_candidates)
    {
        std::vector<NeighborhoodMove> moves;
        moves.reserve(ranked_insertions.size());
        for (const auto& candidate : ranked_insertions)
        {
            moves.push_back(candidate.move);
        }
        return moves;
    }

    std::vector<size_t> selected_indices;
    selected_indices.reserve(max_candidates);
    selected_indices.push_back(0);
    if (selected_indices.size() < max_candidates)
    {
        const size_t last_index = ranked_insertions.size() - 1;
        if (last_index != 0)
        {
            selected_indices.push_back(last_index);
        }
    }

    std::vector<size_t> ranked_indices;
    ranked_indices.reserve(ranked_insertions.size());
    for (size_t i = 0; i < ranked_insertions.size(); ++i)
    {
        if (std::find(selected_indices.begin(), selected_indices.end(), i) == selected_indices.end())
        {
            ranked_indices.push_back(i);
        }
    }

    std::sort(ranked_indices.begin(), ranked_indices.end(),
        [&](const size_t lhs, const size_t rhs) {
            const auto& left = ranked_insertions[lhs];
            const auto& right = ranked_insertions[rhs];
            if (left.score != right.score)
            {
                return left.score < right.score;
            }
            if (left.overrun != right.overrun)
            {
                return left.overrun < right.overrun;
            }
            return left.position < right.position;
        });

    for (const size_t index : ranked_indices)
    {
        if (selected_indices.size() >= max_candidates)
        {
            break;
        }
        selected_indices.push_back(index);
    }

    std::stable_sort(selected_indices.begin(), selected_indices.end(),
        [&](const size_t lhs, const size_t rhs) {
            const auto& left = ranked_insertions[lhs];
            const auto& right = ranked_insertions[rhs];
            if (left.score != right.score)
            {
                return left.score < right.score;
            }
            if (left.overrun != right.overrun)
            {
                return left.overrun < right.overrun;
            }
            return left.position < right.position;
        });

    std::vector<NeighborhoodMove> shortlist;
    shortlist.reserve(selected_indices.size());
    for (const size_t index : selected_indices)
    {
        shortlist.push_back(ranked_insertions[index].move);
    }

    return shortlist;
}

bool has_strong_worker_alternative(const Schedule& schedule,
    const int op,
    const int machine,
    const int current_worker)
{
    const auto& operation_list = *schedule.get_operation_list_ptr();
    const int current_duration = operation_list.worker_duration(op, machine, current_worker);
    if (current_duration <= 0)
    {
        return false;
    }

    int best_alternative_duration = INT_MAX;
    for (const int worker : operation_list.workers_for_machine(op, machine))
    {
        if (worker == current_worker)
        {
            continue;
        }

        const int duration = operation_list.worker_duration(op, machine, worker);
        if (duration <= 0)
        {
            continue;
        }

        best_alternative_duration = std::min(best_alternative_duration, duration);
    }

    if (best_alternative_duration == INT_MAX)
    {
        return false;
    }

    const int minimum_improvement = std::max(1, current_duration / 20);
    return best_alternative_duration + minimum_improvement < current_duration;
}

bool validate_machine_graph(const Graph& graph, int makespan, const char* phase)
{
    if (!awls_trace::enabled())
    {
        return true;
    }

    const int node_num = graph.node_num;
    const int machine_num = static_cast<int>(graph.first_machine_operation.size());
    auto fail = [&](const char* reason, int a = -1, int b = -1, int c = -1) {
        awls_trace::log("GRAPH_INVALID phase=", phase, " reason=", reason,
            " a=", a, " b=", b, " c=", c,
            " node_num=", node_num, " makespan=", makespan);
        return false;
    };

    if (graph.machine_successor.size() != static_cast<size_t>(node_num) ||
        graph.machine_predecessor.size() != static_cast<size_t>(node_num) ||
        graph.on_machine.size() != static_cast<size_t>(node_num))
    {
        return fail("vector_size_mismatch");
    }

    for (int op = 1; op < node_num - 1; ++op)
    {
        const int machine_id = graph.on_machine[op];
        if (machine_id < 0 || machine_id >= machine_num)
        {
            return fail("op_machine_out_of_range", op, machine_id, machine_num);
        }

        const int pred = graph.machine_predecessor[op];
        const int succ = graph.machine_successor[op];
        if (pred != -1)
        {
            if (pred <= 0 || pred >= node_num - 1)
            {
                return fail("pred_out_of_range", op, pred);
            }
            if (graph.machine_successor[pred] != op)
            {
                return fail("pred_succ_mismatch", op, pred, graph.machine_successor[pred]);
            }
            if (graph.on_machine[pred] != machine_id)
            {
                return fail("pred_machine_mismatch", op, pred, machine_id);
            }
        }
        if (succ != -1)
        {
            if (succ <= 0 || succ >= node_num - 1)
            {
                return fail("succ_out_of_range", op, succ);
            }
            if (graph.machine_predecessor[succ] != op)
            {
                return fail("succ_pred_mismatch", op, succ, graph.machine_predecessor[succ]);
            }
            if (graph.on_machine[succ] != machine_id)
            {
                return fail("succ_machine_mismatch", op, succ, machine_id);
            }
        }
    }

    std::vector<int> seen(node_num, 0);
    for (int machine_id = 0; machine_id < machine_num; ++machine_id)
    {
        int count = 0;
        for (int op = graph.first_machine_operation[machine_id]; op != -1; op = graph.machine_successor[op])
        {
            if (op <= 0 || op >= node_num - 1)
            {
                return fail("chain_op_out_of_range", machine_id, op);
            }
            if (++seen[op] > 1)
            {
                return fail("chain_duplicate_or_cycle", machine_id, op, seen[op]);
            }
            if (graph.on_machine[op] != machine_id)
            {
                return fail("chain_machine_mismatch", machine_id, op, graph.on_machine[op]);
            }
            if (++count > node_num)
            {
                return fail("chain_too_long", machine_id, count);
            }
        }
        if (machine_id < static_cast<int>(graph.machine_operation_count.size()) &&
            graph.machine_operation_count[machine_id] != count)
        {
            return fail("machine_count_mismatch", machine_id, count, graph.machine_operation_count[machine_id]);
        }
    }

    return true;
}

void validate_machine_graph_or_exit(const Graph& graph, int makespan, const char* phase)
{
    if (!validate_machine_graph(graph, makespan, phase))
    {
        std::exit(86);
    }
}

bool is_real_operation(const Schedule& schedule, const int op)
{
    return op > 0 && op < schedule.get_graph().node_num - 1;
}

void add_start_candidate(const Schedule& schedule, std::vector<int>& start_candidates, const int op)
{
    if (!is_real_operation(schedule, op))
    {
        return;
    }
    if (!schedule.is_critical(op))
    {
        return;
    }
    if (schedule.get_time_info()[op].forward_path_length != 0)
    {
        return;
    }
    if (std::find(start_candidates.begin(), start_candidates.end(), op) == start_candidates.end())
    {
        start_candidates.push_back(op);
    }
}

std::vector<int> collect_start_candidates(const Schedule& schedule)
{
    std::vector<int> start_candidates;
    const auto& graph = schedule.get_graph();
    start_candidates.reserve(graph.first_machine_operation.size() + graph.first_worker_operation.size());

    for (const int op : graph.first_machine_operation)
    {
        add_start_candidate(schedule, start_candidates, op);
    }

    if (schedule.get_operation_list_ptr()->has_workers())
    {
        for (const int op : graph.first_worker_operation)
        {
            add_start_candidate(schedule, start_candidates, op);
        }
    }

    return start_candidates;
}

void add_critical_successor(const Schedule& schedule, const int op, const int successor, std::vector<int>& successors)
{
    if (!is_real_operation(schedule, successor))
    {
        return;
    }
    if (!schedule.is_critical(successor))
    {
        return;
    }
    if (schedule.get_time_info()[successor].forward_path_length != schedule.get_time_info()[op].end_time)
    {
        return;
    }
    if (std::find(successors.begin(), successors.end(), successor) == successors.end())
    {
        successors.push_back(successor);
    }
}

std::vector<int> collect_critical_successors(const Schedule& schedule, const int op)
{
    std::vector<int> successors;
    successors.reserve(3);
    const auto& graph = schedule.get_graph();

    add_critical_successor(schedule, op, graph.machine_successor[op], successors);
    if (schedule.get_operation_list_ptr()->has_workers())
    {
        add_critical_successor(schedule, op, graph.worker_successor[op], successors);
    }
    add_critical_successor(schedule, op, graph.job_successor[op], successors);

    return successors;
}

void flush_critical_block(const CriticalBlockResource resource,
    const int resource_id,
    std::vector<int>& ops,
    std::vector<CriticalBlock>& blocks,
    std::set<std::pair<int, std::vector<int>>>& unique_blocks)
{
    if (ops.size() <= 1)
    {
        ops.clear();
        return;
    }

    std::pair<int, std::vector<int>> key{ static_cast<int>(resource), ops };
    if (!unique_blocks.contains(key))
    {
        blocks.push_back({ resource, resource_id, ops });
        unique_blocks.insert(std::move(key));
    }
    ops.clear();
}

void append_blocks_from_path(const Schedule& schedule,
    const std::vector<int>& path,
    std::vector<CriticalBlock>& blocks,
    std::set<std::pair<int, std::vector<int>>>& unique_blocks)
{
    const auto append_for_resource = [&](const CriticalBlockResource resource) {
        const auto& graph = schedule.get_graph();
        int prev_resource_id = -1;
        std::vector<int> block_ops;

        for (const int op : path)
        {
            const int resource_id = resource == CriticalBlockResource::Machine
                ? graph.on_machine[op]
                : graph.on_worker[op];

            if (resource_id < 0)
            {
                flush_critical_block(resource, prev_resource_id, block_ops, blocks, unique_blocks);
                prev_resource_id = -1;
                continue;
            }

            if (block_ops.empty() || resource_id == prev_resource_id)
            {
                block_ops.push_back(op);
                prev_resource_id = resource_id;
            }
            else
            {
                flush_critical_block(resource, prev_resource_id, block_ops, blocks, unique_blocks);
                block_ops.push_back(op);
                prev_resource_id = resource_id;
            }
        }

        flush_critical_block(resource, prev_resource_id, block_ops, blocks, unique_blocks);
    };

    append_for_resource(CriticalBlockResource::Machine);
    if (schedule.get_operation_list_ptr()->has_workers())
    {
        append_for_resource(CriticalBlockResource::Worker);
    }
}

void append_blocks_from_resource_chains(const Schedule& schedule,
    const CriticalBlockResource resource,
    std::vector<CriticalBlock>& blocks,
    std::set<std::pair<int, std::vector<int>>>& unique_blocks)
{
    const auto& graph = schedule.get_graph();
    const auto& first_ops = resource == CriticalBlockResource::Machine
        ? graph.first_machine_operation
        : graph.first_worker_operation;

    for (int resource_id = 0; resource_id < static_cast<int>(first_ops.size()); ++resource_id)
    {
        std::vector<int> block_ops;
        int curr_op = first_ops[resource_id];
        while (curr_op != -1)
        {
            if (!schedule.is_critical(curr_op))
            {
                flush_critical_block(resource, resource_id, block_ops, blocks, unique_blocks);
                curr_op = resource == CriticalBlockResource::Machine
                    ? graph.machine_successor[curr_op]
                    : graph.worker_successor[curr_op];
                continue;
            }

            if (block_ops.empty())
            {
                block_ops.push_back(curr_op);
            }
            else
            {
                const int prev_op = block_ops.back();
                if (schedule.get_time_info()[prev_op].end_time == schedule.get_time_info()[curr_op].forward_path_length)
                {
                    block_ops.push_back(curr_op);
                }
                else
                {
                    flush_critical_block(resource, resource_id, block_ops, blocks, unique_blocks);
                    block_ops.push_back(curr_op);
                }
            }

            curr_op = resource == CriticalBlockResource::Machine
                ? graph.machine_successor[curr_op]
                : graph.worker_successor[curr_op];
        }

        flush_critical_block(resource, resource_id, block_ops, blocks, unique_blocks);
    }
}
}

// weight update (per-op) Algorithm 3 - �޸��������Ϳ��м��������߼�
// �޸ĺ��UpdateWeight_per_op����
void UpdateWeight_per_op(std::vector<Operation>& operations,
    int movedOp,
    const std::vector<int>& criticalPathOps,
    int fStar, int fPrev, int fCurr,
    int beta, int gamma, int theta) {

    if (fCurr >= fPrev) {
        // ���±��ƶ�������Ȩ��
        if (operations[movedOp].t > beta)
            operations[movedOp].w = 0;
        else
            operations[movedOp].w++;
    }

    if (fCurr >= fPrev) {
        // ���±��ƶ������Ŀ��м���
        if (operations[movedOp].t > gamma) {
            operations[movedOp].t = gamma;
        }
        else {
            operations[movedOp].t = std::max(operations[movedOp].t - theta, 0);
        }

        // ���������ǹؼ�·�������Ŀ��м���
        for (int o = 1; o < (int)operations.size() - 1; ++o) {
            // ��������ڵ㣨����0�����һ����
            bool isCritical = std::find(criticalPathOps.begin(), criticalPathOps.end(), o) != criticalPathOps.end();
            if (!isCritical && o != movedOp) {
                operations[o].t = operations[o].t + 1;
            }
        }
    }
    else {
        // �����ǰ���ǰһ����ã��������в����Ŀ��м���
        for (int o = 1; o < (int)operations.size() - 1; ++o) {
            operations[o].t++;
        }
    }

    if (fCurr < fStar) {
        // ����ҵ��µ����Ž⣬�������в�����Ȩ�غͿ��м���
        for (int o = 1; o < (int)operations.size() - 1; ++o) {
            operations[o].t = INT_MAX;  // Reset to +��
            operations[o].w = 0;        // Reset to 0
        }
    }
}

void TabuSearch::search(const Schedule& schedule, const std::atomic<bool>& stop_flag)
{
    awls_trace::log("TabuSearch::search start makespan=", schedule.get_makespan());
    current_schedule = schedule;
    best_schedule = schedule;
    validate_machine_graph_or_exit(current_schedule.graph, current_schedule.get_makespan(), "search_start");
    Schedule prev_schedule;
    iteration = 0;
    while (iteration < kMaxTabuIterationsPerPass)
    {
        if (iteration < 5 || iteration % 500 == 0)
        {
            awls_trace::log("TabuSearch::search iteration=", iteration,
                " current=", current_schedule.get_makespan(),
                " best=", best_schedule.get_makespan());
        }
        if (stop_flag.load(std::memory_order_relaxed))
        {
            break;
        }
        auto move = find_move(&stop_flag);
        if (move.which == 0)
        {
            awls_trace::log("TabuSearch::search empty move iteration=", iteration);
            break;
        }
        if (iteration < 5 || iteration % 500 == 0)
        {
            awls_trace::log("TabuSearch::search picked move iteration=", iteration,
                " method=", static_cast<int>(move.method),
                " which=", move.which,
                " where=", move.where);
        }
        prev_schedule = current_schedule;
        make_move(move);
        validate_machine_graph_or_exit(current_schedule.graph, current_schedule.get_makespan(), "after_make_move");
        if (iteration < 5 || iteration % 500 == 0)
        {
            awls_trace::log("TabuSearch::search after make_move iteration=", iteration,
                " current=", current_schedule.get_makespan(),
                " best=", best_schedule.get_makespan());
        }
        if (iteration < 5 && awls_trace::enabled())
        {
            for (int machine_id = 0; machine_id < static_cast<int>(current_schedule.graph.first_machine_operation.size()); ++machine_id)
            {
                std::ostringstream oss;
                oss << "TabuSearch::search machine_chain iteration=" << iteration
                    << " machine=" << machine_id << " chain=";
                int guard = 0;
                for (int op = current_schedule.graph.first_machine_operation[machine_id];
                    op != -1 && guard < current_schedule.graph.node_num + 2;
                    op = current_schedule.graph.machine_successor[op], ++guard)
                {
                    if (guard > 0)
                    {
                        oss << "->";
                    }
                    oss << op;
                }
                if (guard >= current_schedule.graph.node_num + 2)
                {
                    oss << "->...";
                }
                awls_trace::log(oss.str());
            }
            for (int op = 1; op < current_schedule.graph.node_num - 1; ++op)
            {
                awls_trace::log("TabuSearch::search op_state iteration=", iteration,
                    " op=", op,
                    " machine=", current_schedule.graph.on_machine[op],
                    " mp=", current_schedule.graph.machine_predecessor[op],
                    " ms=", current_schedule.graph.machine_successor[op],
                    " js=", current_schedule.graph.job_successor[op],
                    " start=", current_schedule.time_info[op].forward_path_length,
                    " end=", current_schedule.time_info[op].end_time);
            }
        }
         // ��ȡ�ؼ�·������
        std::vector<int> critical_ops;
        for (int i = 1; i < current_schedule.graph.node_num - 1; ++i) {
            if (current_schedule.is_critical_operation(i)) {
                critical_ops.push_back(i);
            }
        }
        
        // ���±��ƶ�������Ȩ�غͿ��м���
        if (kEnableAdaptiveWeightUpdate)
        {
            UpdateWeight_per_op(current_schedule.operation_list->operations,
                move.which,
                critical_ops,
                best_schedule.makespan,  // fStar - ��ʷ����makespan
                prev_schedule.makespan,  // fPrev - ǰһ�����makespan
                current_schedule.makespan, // fCurr - ��ǰ���makespan
                Schedule::beta,
                Schedule::gamma,
                Schedule::theta);
        }
        
        

        if (current_schedule.makespan < best_schedule.makespan)
        {
            best_schedule = current_schedule;
        }
        if (iteration % 1000 == 0)
        {
            // ���ڼ���Ƿ��ֹͣ
            if (stop_flag.load(std::memory_order_relaxed))
            {
                break;
            }
        }

#ifdef PRINT_INFO
        if (iteration % 1000 == 0)
        {
            std::cerr << iteration << " " << current_schedule.makespan << " " << best_schedule.makespan << " "
                << move.str() << " " << move.which << " " << move.where << std::endl;
        }
#endif
        iteration++;
    }
    awls_trace::log("TabuSearch::search finish best=", best_schedule.get_makespan(),
        " iterations=", iteration);
}

void TabuSearch::change_machine_evaluate_and_push(const Schedule& schedule, const NeighborhoodMove& move,
    const std::vector<int>& intersection,
    std::vector<NeighborhoodMove>& all_moves,
    std::vector<NeighborhoodMove>& best_moves, int& min_makespan) const
{
    all_moves.push_back(move);
    int val = change_machine_evaluate(schedule, move, intersection);
    if (!is_tabu(move, val))
    {
        if (val < min_makespan)
        {
            min_makespan = val;
            best_moves.clear();
            best_moves.push_back(move);
        }
        else if (val == min_makespan)
        {
            best_moves.push_back(move);
        }
    }
}

void TabuSearch::same_machine_evaluate_and_push(const Schedule& schedule, const NeighborhoodMove& move,
    std::vector<NeighborhoodMove>& all_moves,
    std::vector<NeighborhoodMove>& best_moves, int& min_makespan) const
{
    if (!schedule.is_legal_move(move))
    {
        return;
    }
    all_moves.push_back(move);
    int val = same_machine_evaluate(schedule, move);
    if (!is_tabu(move, val))
    {
        if (val < min_makespan)
        {
            min_makespan = val;
            best_moves.clear();
            best_moves.push_back(move);
        }
        else if (val == min_makespan)
        {
            best_moves.push_back(move);
        }
    }
}


NeighborhoodMove TabuSearch::find_move(const std::atomic<bool>* stop_flag)
{
    awls_trace::log("TabuSearch::find_move begin current=", current_schedule.get_makespan());
    std::vector<NeighborhoodMove> all_moves;
    std::vector<NeighborhoodMove> best_moves;
    std::vector<char> machine_block_worker_probe(current_schedule.graph.node_num, 0);
    bool find_all = false;
    int min_makespan = INT_MAX;
    while (true)
    {
        if (should_stop_search(stop_flag))
        {
            return {};
        }
        if (!find_all)
            update_critical_block();
        else
            update_all_critical_block();
        awls_trace::log("TabuSearch::find_move blocks=", critical_blocks.size(),
            " find_all=", find_all ? 1 : 0);
        for (const auto& block : critical_blocks)
        {
            if (should_stop_search(stop_flag))
            {
                return {};
            }
            if (block.operations.empty())
            {
                continue;
            }
            if (awls_trace::enabled())
            {
                std::ostringstream oss;
                oss << "TabuSearch::find_move block resource=" << static_cast<int>(block.resource)
                    << " id=" << block.resource_id << " ops=";
                for (size_t i = 0; i < block.operations.size(); ++i)
                {
                    if (i > 0)
                    {
                        oss << ",";
                    }
                    oss << block.operations[i];
                }
                awls_trace::log(oss.str());
            }
            if (block.resource == CriticalBlockResource::Machine)
            {
                for (int i = 0; i < static_cast<int>(block.operations.size()) && !should_stop_search(stop_flag); i++)
                {
                    const int op = block.operations[i];
                    if (op <= 0 || op >= current_schedule.graph.node_num - 1)
                    {
                        continue;
                    }
                    const int machine_id = current_schedule.graph.on_machine[op];
                    if (machine_id < 0 || machine_id >= static_cast<int>(current_schedule.graph.first_machine_operation.size()))
                    {
                        continue;
                    }

                    for (int cur_op = current_schedule.graph.first_machine_operation[machine_id];
                        cur_op != -1 && cur_op != block.operations.front() && kEnablePrefixFrontMoves && !should_stop_search(stop_flag);
                        cur_op = current_schedule.graph.machine_successor[cur_op])
                    {
                        NeighborhoodMove move{ Method::FRONT, op, cur_op };
                        awls_trace::log("TabuSearch::find_move eval FRONT which=", move.which, " where=", move.where);
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                        awls_trace::log("TabuSearch::find_move eval FRONT done which=", move.which, " where=", move.where);
                    }


                    for (int cur_op = current_schedule.graph.machine_successor[block.operations.back()];
                        cur_op != -1 && !should_stop_search(stop_flag);
                        cur_op = current_schedule.graph.machine_successor[cur_op])
                    {
                        NeighborhoodMove move{ Method::BACK, op, cur_op };
                        awls_trace::log("TabuSearch::find_move eval BACK which=", move.which, " where=", move.where);
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                        awls_trace::log("TabuSearch::find_move eval BACK done which=", move.which, " where=", move.where);
                    }

                    const bool is_block_endpoint = block.operations.size() <= 2 ||
                        i == 0 || i + 1 == static_cast<int>(block.operations.size());
                    const int current_worker = current_schedule.graph.on_worker[op];
                    const bool has_strong_alternative = has_strong_worker_alternative(
                        current_schedule, op, machine_id, current_worker);
                    if (kEnableWorkerChangeNeighborhood && current_schedule.operation_list->has_workers() &&
                        (is_block_endpoint || has_strong_alternative) && !machine_block_worker_probe[op])
                    {
                        machine_block_worker_probe[op] = 1;
                        const size_t shortlist_size =
                            has_strong_alternative && !is_block_endpoint
                            ? worker_change_strong_shortlist_size
                            : worker_change_shortlist_size;
                        const auto shortlist = collect_worker_shortlist(
                            current_schedule, op, machine_id, current_worker, shortlist_size);
                        for (const int worker : shortlist)
                        {
                            if (should_stop_search(stop_flag))
                            {
                                return {};
                            }

                            NeighborhoodMove move{ Method::CHANGE_WORKER, op, worker };
                            same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                        }
                    }
                }

                if (const int n = static_cast<int>(block.operations.size()); n == 2)
                {
                    if (kEnableTwoBlockBackMove &&
                        block.operations[0] > 0 && block.operations[0] < current_schedule.graph.node_num - 1 &&
                        block.operations[1] > 0 && block.operations[1] < current_schedule.graph.node_num - 1)
                    {
                        NeighborhoodMove move{ Method::BACK, block.operations[0], block.operations[1] };
                        awls_trace::log("TabuSearch::find_move eval block2 BACK which=", move.which, " where=", move.where);
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                        awls_trace::log("TabuSearch::find_move eval block2 BACK done which=", move.which, " where=", move.where);
                    }
                }
                else
                {
                    for (int j = 2; j < n && kEnableFrontToBackMoves && !should_stop_search(stop_flag); j++)
                    {
                        if (block.operations.front() <= 0 || block.operations.front() >= current_schedule.graph.node_num - 1 ||
                            block.operations[j] <= 0 || block.operations[j] >= current_schedule.graph.node_num - 1)
                        {
                            continue;
                        }
                        NeighborhoodMove move(Method::BACK, block.operations.front(), block.operations[j]);
                        awls_trace::log("TabuSearch::find_move eval front_to_back which=", move.which, " where=", move.where);
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                        awls_trace::log("TabuSearch::find_move eval front_to_back done which=", move.which, " where=", move.where);
                    }
                    for (int j = n - 2; j >= 0 && kEnableBackToFrontMoves && !should_stop_search(stop_flag); j--)
                    {
                        if (block.operations.back() <= 0 || block.operations.back() >= current_schedule.graph.node_num - 1 ||
                            block.operations[j] <= 0 || block.operations[j] >= current_schedule.graph.node_num - 1)
                        {
                            continue;
                        }
                        NeighborhoodMove move(Method::FRONT, block.operations.back(), block.operations[j]);
                        awls_trace::log("TabuSearch::find_move eval back_to_front which=", move.which, " where=", move.where);
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                        awls_trace::log("TabuSearch::find_move eval back_to_front done which=", move.which, " where=", move.where);
                    }
                    if (kEnableMiddleBlockMoves)
                    {
                        for (int j = 1; j < n - 1 && !should_stop_search(stop_flag); j++)
                        {
                            if (block.operations[j] <= 0 || block.operations[j] >= current_schedule.graph.node_num - 1 ||
                                block.operations.front() <= 0 || block.operations.front() >= current_schedule.graph.node_num - 1)
                            {
                                continue;
                            }
                            NeighborhoodMove move(Method::FRONT, block.operations[j], block.operations.front());
                            awls_trace::log("TabuSearch::find_move eval middle_to_front which=", move.which, " where=", move.where);
                            same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                            awls_trace::log("TabuSearch::find_move eval middle_to_front done which=", move.which, " where=", move.where);
                        }
                    }
                    if (kEnableMiddleBlockMoves)
                    {
                        for (int j = 1; j < n - 1 && !should_stop_search(stop_flag); j++)
                        {
                            if (block.operations[j] <= 0 || block.operations[j] >= current_schedule.graph.node_num - 1 ||
                                block.operations.back() <= 0 || block.operations.back() >= current_schedule.graph.node_num - 1)
                            {
                                continue;
                            }
                            NeighborhoodMove move(Method::BACK, block.operations[j], block.operations.back());
                            awls_trace::log("TabuSearch::find_move eval middle_to_back which=", move.which, " where=", move.where);
                            same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                            awls_trace::log("TabuSearch::find_move eval middle_to_back done which=", move.which, " where=", move.where);
                        }
                    }
                }

            }
            else if (block.resource == CriticalBlockResource::Worker && kEnableWorkerChangeNeighborhood)
            {
                for (const int op : block.operations)
                {
                    if (should_stop_search(stop_flag))
                    {
                        return {};
                    }
                    if (!current_schedule.is_critical_operation(op))
                    {
                        continue;
                    }

                    const int machine = current_schedule.graph.on_machine[op];
                    const int current_worker = current_schedule.graph.on_worker[op];
                    const auto& worker_candidates = current_schedule.operation_list->workers_for_machine(op, machine);
                    for (const int worker : worker_candidates)
                    {
                        if (should_stop_search(stop_flag))
                        {
                            return {};
                        }
                        if (worker == current_worker)
                        {
                            continue;
                        }

                        NeighborhoodMove move{ Method::CHANGE_WORKER, op, worker };
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                    }
                }
            }
        }

        if (!find_all && kEnableChangeMachineNeighborhood)
        {
            for (int op = 1; op < current_schedule.graph.node_num - 1 && !should_stop_search(stop_flag); ++op)
            {
                if (!current_schedule.is_critical_operation(op))
                {
                    continue;
                }

                const int old_machine = current_schedule.graph.on_machine[op];
                const auto candidate_machines = collect_machine_shortlist(
                    current_schedule, op, old_machine, machine_change_shortlist_size);
                for (const int candidate_machine : candidate_machines)
                {
                    if (should_stop_search(stop_flag))
                    {
                        return {};
                    }

                    const auto insertion_moves = collect_machine_insertion_shortlist(
                        current_schedule, op, candidate_machine, machine_change_position_shortlist_size);
                    for (const auto& move : insertion_moves)
                    {
                        if (should_stop_search(stop_flag))
                        {
                            return {};
                        }
                        same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                    }
                }
            }
        }

        if (all_moves.empty())
        {
            if (find_all)
            {
                awls_trace::log("TabuSearch::find_move no admissible move after full scan current=",
                    current_schedule.get_makespan());
                return {};
            }
            find_all = true;
            continue;
        }
        break;
    }


    if (best_moves.empty())
    {
        // �������һ������
        const auto selected = all_moves[RAND_INT(static_cast<int>(all_moves.size()))];
        awls_trace::log("TabuSearch::find_move fallback method=", static_cast<int>(selected.method),
            " which=", selected.which,
            " where=", selected.where,
            " all_moves=", all_moves.size(),
            " min_makespan=", min_makespan);
        return selected;
    }
    if (min_makespan > current_schedule.makespan && RAND_INT(100) < 3)
    {
        const auto selected = all_moves[RAND_INT(static_cast<int>(all_moves.size()))];
        awls_trace::log("TabuSearch::find_move diversified method=", static_cast<int>(selected.method),
            " which=", selected.which,
            " where=", selected.where,
            " all_moves=", all_moves.size(),
            " min_makespan=", min_makespan);
        return selected;
    }
    // ���������������������һ������
    const int index = RAND_INT(static_cast<int>(best_moves.size()));
    const auto selected = best_moves[index];
    awls_trace::log("TabuSearch::find_move selected method=", static_cast<int>(selected.method),
        " which=", selected.which,
        " where=", selected.where,
        " best_moves=", best_moves.size(),
        " min_makespan=", min_makespan);
    return selected;
}
#define NEXT_MACHINE_OP(op) current_schedule.graph.machine_successor[op]

void TabuSearch::make_move(const NeighborhoodMove& move)
{
    awls_trace::log("TabuSearch::make_move start method=", static_cast<int>(move.method),
        " which=", move.which, " where=", move.where);
    if (move.method == Method::CHANGE_WORKER)
    {
        const int previous_worker =
            move.which >= 0 && move.which < static_cast<int>(current_schedule.graph.on_worker.size())
            ? current_schedule.graph.on_worker[move.which]
            : -1;
        if (enable_worker_tabu && move.which >= 0 &&
            move.which < static_cast<int>(worker_tabu.size()) && previous_worker >= 0)
        {
            const unsigned long long tabu_time = iteration + RandomGenerator::instance().getInt(L, static_cast<int>(L_max));
            worker_tabu[move.which].insert_or_assign(previous_worker, tabu_time);
        }
        current_schedule.make_move(move);
        awls_trace::log("TabuSearch::make_move finish current=", current_schedule.get_makespan());
        return;
    }
    // ��ӽ���
    std::vector<int> tabu_sequence;
    int pos = 0;
    const int machine_id = current_schedule.graph.on_machine[move.which];

    if (move.method == Method::FRONT)
    {
        // Forward move: collect operations from 'where' to new position after move
        for (int op = move.where; op != NEXT_MACHINE_OP(move.which); op = NEXT_MACHINE_OP(op))
        {
            tabu_sequence.push_back(op);
        }
        pos = current_schedule.graph.on_machine_pos_vec[move.where];
    }
    else if (move.method == Method::BACK)
    {
        // Backward move: collect operations from 'which' to new position after move
        for (int op = move.which; op != NEXT_MACHINE_OP(move.where); op = NEXT_MACHINE_OP(op))
        {
            tabu_sequence.push_back(op);
        }
        pos = current_schedule.graph.on_machine_pos_vec[move.which];
    }
    else
    {
        pos = current_schedule.graph.on_machine_pos_vec[move.which];
        const int mp_which = current_schedule.graph.machine_predecessor[move.which];
        const int ms_which = current_schedule.graph.machine_successor[move.which];
        if (mp_which != -1)
        {
            tabu_sequence.push_back(mp_which);
            pos--;
        }
        tabu_sequence.push_back(move.which);
        if (ms_which != -1)
        {
            tabu_sequence.push_back(ms_which);
        }
    }

    // Calculate dynamic tabu tenure
    const unsigned long long tabu_time = iteration + RandomGenerator::instance().getInt(L, static_cast<int>(L_max));
#ifdef TABU_WITH_POS
    tabu_list.add_sequence(machine_id, pos, tabu_sequence, tabu_time);
#else
    tabu_list.add_sequence(machine_id, tabu_sequence, tabu_time);
#endif

    current_schedule.make_move(move);
    awls_trace::log("TabuSearch::make_move finish current=", current_schedule.get_makespan());
}

bool TabuSearch::is_tabu(const NeighborhoodMove& move, const int makespan) const
{
    // ����׼��
    if (makespan < best_schedule.makespan)
    {
        return false;
    }
    if (move.method == Method::CHANGE_WORKER)
    {
        if (!enable_worker_tabu)
        {
            return false;
        }
        if (move.which < 0 || move.which >= static_cast<int>(worker_tabu.size()))
        {
            return false;
        }
        const auto it = worker_tabu[move.which].find(move.where);
        return it != worker_tabu[move.which].end() && it->second >= iteration;
    }
    std::vector<int> new_op_sequence;
    int pos = 0;
    const int machine_id = current_schedule.graph.on_machine[move.where];
    if (move.method == Method::FRONT)
    {
        new_op_sequence.push_back(move.which);
        for (auto op = move.where; op != move.which; op = NEXT_MACHINE_OP(op))
        {
            new_op_sequence.push_back(op);
        }
        // �ҵ���where��ʼ�Ļ����ϵ�λ��
        pos = current_schedule.graph.on_machine_pos_vec[move.where];
    }
    else if (move.method == Method::BACK)
    {
        for (auto op = NEXT_MACHINE_OP(move.which); op != NEXT_MACHINE_OP(move.where); op = NEXT_MACHINE_OP(op))
        {
            new_op_sequence.push_back(op);
        }
        new_op_sequence.push_back(move.which);
        // �ҵ�ԭ�ȵ�which�Ļ����ϵ�λ��
        pos = current_schedule.graph.on_machine_pos_vec[move.which];
    }
    else
    {
        if (move.method == Method::CHANGE_MACHINE_BACK)
        {
            const int u = move.where;
            const int v = NEXT_MACHINE_OP(u);
            new_op_sequence.push_back(u);
            new_op_sequence.push_back(move.which);
            if (v != -1)
            {
                new_op_sequence.push_back(v);
            }
            // �ҵ�u�ڻ����ϵ�λ��
            pos = current_schedule.graph.on_machine_pos_vec[u];
        }
        else
        {
            const int v = move.where;
            // �ҵ�v�ڻ����ϵ�λ��
            pos = current_schedule.graph.on_machine_pos_vec[v];
            if (const int u = current_schedule.graph.machine_predecessor[v]; u != -1)
            {
                new_op_sequence.push_back(u);
                pos--;
            }
            new_op_sequence.push_back(move.which);
            new_op_sequence.push_back(v);
        }
    }
#ifdef TABU_WITH_POS
    return tabu_list.is_tabu(machine_id, pos, new_op_sequence, iteration);
#else
    return tabu_list.is_tabu(machine_id, new_op_sequence, iteration);
#endif
}

void TabuSearch::update_critical_block()
{
    const auto& graph = current_schedule.graph;
    awls_trace::log("update_critical_block start makespan=", current_schedule.get_makespan());

    std::vector<int> start_candidates = collect_start_candidates(current_schedule);

    if (!start_candidates.empty())
    {
        awls_trace::log("update_critical_block start_candidates=", start_candidates.size());
        do
        {
            critical_path.clear();
            // �����һ��start��ʼѰ��
            int start_index = RAND_INT(static_cast<int>(start_candidates.size()));
            critical_path.push_back(start_candidates[start_index]);
            awls_trace::log("update_critical_block picked_start=", critical_path.back(),
                " remaining_candidates=", start_candidates.size());
            start_candidates[start_index] = start_candidates.back();
            start_candidates.pop_back();
            int step = 0;
            while (current_schedule.time_info[critical_path.back()].end_time != current_schedule.makespan)
            {
                const auto op = critical_path.back();
                const auto ms_op = graph.machine_successor[op];
                const auto ws_op = current_schedule.operation_list->has_workers() ? graph.worker_successor[op] : -1;
                const auto js_op = graph.job_successor[op];
                awls_trace::log("update_critical_block step=", step,
                    " op=", op,
                    " end=", current_schedule.time_info[op].end_time,
                    " ms=", ms_op,
                    " ws=", ws_op,
                    " js=", js_op);
                if (++step > current_schedule.graph.node_num * 2)
                {
                    awls_trace::log("update_critical_block exceeded guard with path size=", critical_path.size());
                    break;
                }

                std::vector<int> successors = collect_critical_successors(current_schedule, op);
                if (successors.empty())
                {
                    break;
                }
                critical_path.push_back(successors[RAND_INT(static_cast<int>(successors.size()))]);
            }

#ifdef UNIT_TEST
        // ���Թؼ�·��
        assert(current_schedule.is_critical_operation(critical_path.front()));
        assert(current_schedule.is_critical_operation(critical_path.back()));
        assert(current_schedule.time_info[critical_path.front()].forward_path_length == 0);
        assert(current_schedule.time_info[critical_path.back()].end_time == current_schedule.makespan);
        for (auto i = 0; i < critical_path.size() - 1; i++)
        {
            assert(current_schedule.is_critical_operation(critical_path[i]));
            assert(current_schedule.time_info[critical_path[i]].end_time ==
                current_schedule.time_info[critical_path[i + 1]].forward_path_length);
            }
#endif
            critical_blocks.clear();
            std::set<std::pair<int, std::vector<int>>> unique_blocks;
            append_blocks_from_path(current_schedule, critical_path, critical_blocks, unique_blocks);
            awls_trace::log("update_critical_block built_path_size=", critical_path.size(),
                " blocks=", critical_blocks.size());
        } while (critical_blocks.empty() && !start_candidates.empty());
    }
    if (critical_blocks.empty())
    {
        std::set<std::pair<int, std::vector<int>>> unique_blocks;
        append_blocks_from_resource_chains(current_schedule, CriticalBlockResource::Machine, critical_blocks, unique_blocks);
        if (current_schedule.operation_list->has_workers())
        {
            append_blocks_from_resource_chains(current_schedule, CriticalBlockResource::Worker, critical_blocks, unique_blocks);
        }
    }
#ifdef UNIT_TEST
    for (const auto& block : critical_blocks)
    {
        for (int i = 0; i < static_cast<int>(block.operations.size()) - 1; i++)
        {
            assert(current_schedule.time_info[block.operations[i]].end_time ==
                current_schedule.time_info[block.operations[i + 1]].forward_path_length);
            if (block.resource == CriticalBlockResource::Machine)
            {
                assert(current_schedule.graph.on_machine[block.operations[i]] ==
                    current_schedule.graph.on_machine[block.operations[i + 1]]);
            }
            else
            {
                assert(current_schedule.graph.on_worker[block.operations[i]] ==
                    current_schedule.graph.on_worker[block.operations[i + 1]]);
            }
        }
    }
#endif
}


void TabuSearch::update_all_critical_block()
{
    std::vector<std::vector<int>> all_paths;
    const auto& graph = current_schedule.graph;
    all_paths.reserve((graph.first_machine_operation.size() + graph.first_worker_operation.size()) * 2);

    for (const auto op : collect_start_candidates(current_schedule))
    {
        all_paths.emplace_back(1, op);
    }

    for (int i = 0; i < all_paths.size(); ++i)
    {
        auto& current_path = all_paths[i];
        int op = current_path.back();
        while (current_schedule.time_info[op].end_time < current_schedule.makespan)
        {
            std::vector<int> successors = collect_critical_successors(current_schedule, op);
            if (successors.empty())
            {
                break;
            }

            current_path.push_back(successors.front());
            for (size_t succ_idx = 1; succ_idx < successors.size(); ++succ_idx)
            {
                all_paths.push_back(current_path);
                all_paths.back().back() = successors[succ_idx];
            }
            op = current_path.back();
        }
    }

    critical_blocks.clear();
    std::set<std::pair<int, std::vector<int>>> unique_blocks;

    for (const auto& path : all_paths)
    {
        append_blocks_from_path(current_schedule, path, critical_blocks, unique_blocks);
    }

    if (critical_blocks.empty())
    {
        append_blocks_from_resource_chains(current_schedule, CriticalBlockResource::Machine, critical_blocks, unique_blocks);
        if (current_schedule.operation_list->has_workers())
        {
            append_blocks_from_resource_chains(current_schedule, CriticalBlockResource::Worker, critical_blocks, unique_blocks);
        }
    }
}
