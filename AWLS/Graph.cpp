//
// Created by qiming on 25-4-12.
//
#include "Graph.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include "Instance.h"
#include "RandomGenerator.h"

/**
 * Perform topological sort on the graph
 * @param reverse If true, perform reverse topological sort
 * @return A deque containing nodes in topologically sorted order
 * @throws std::runtime_error if graph contains a cycle
 */
std::deque<int> Graph::topological_sort(bool reverse, const bool include_worker) const
{
    std::vector<int> in_degree(node_num, 0);
    int first_node = reverse ? node_num - 1 : 0;

    // Calculate in-degree for each node
    for (int i = 0; i < node_num; ++i)
    {
        if (reverse)
        {
            in_degree[i] = (machine_successor[i] != -1) + (job_successor[i] != -1);
            if (include_worker)
            {
                in_degree[i] += static_cast<int>(worker_successor[i] != -1);
            }
        }
        else
        {
            in_degree[i] = static_cast<int>(machine_predecessor[i] != -1) + static_cast<int>(job_predecessor[i] != -1);
            if (include_worker)
            {
                in_degree[i] += static_cast<int>(worker_predecessor[i] != -1);
            }
        }
    }

    std::deque<int> result; // Topological order result
    std::deque<int> candidates; // Nodes with zero in-degree
    candidates.push_back(first_node);

    while (true)
    {
        if (candidates.empty())
        {
            throw std::runtime_error("Graph contains a cycle");
        }

        int curr = candidates.front();
        candidates.pop_front();
        result.push_back(curr);

        if (result.size() == node_num - 1)
        {
            if (reverse)
            {
                result.push_back(0);
            }
            else
            {
                result.push_back(node_num - 1);
            }
            break;
        }

        if (curr == first_node)
        {
            if (reverse)
            {
                for (const int node : last_job_operation)
                {
                    in_degree[node]--;
                    if (in_degree[node] == 0)
                    {
                        candidates.push_back(node);
                    }
                }
            }
            else
            {
                for (const int node : first_job_operation)
                {
                    in_degree[node]--;
                    if (in_degree[node] == 0)
                    {
                        candidates.push_back(node);
                    }
                }
            }
        }
        else
        {
            if (reverse)
            {
                if (job_predecessor[curr] != -1)
                {
                    in_degree[job_predecessor[curr]]--;
                    if (in_degree[job_predecessor[curr]] == 0)
                    {
                        candidates.push_back(job_predecessor[curr]);
                    }
                }

                if (machine_predecessor[curr] != -1)
                {
                    in_degree[machine_predecessor[curr]]--;
                    if (in_degree[machine_predecessor[curr]] == 0)
                    {
                        candidates.push_back(machine_predecessor[curr]);
                    }
                }
                if (include_worker && worker_predecessor[curr] != -1)
                {
                    in_degree[worker_predecessor[curr]]--;
                    if (in_degree[worker_predecessor[curr]] == 0)
                    {
                        candidates.push_back(worker_predecessor[curr]);
                    }
                }
            }
            else
            {
                if (job_successor[curr] != -1)
                {
                    in_degree[job_successor[curr]]--;
                    if (in_degree[job_successor[curr]] == 0)
                    {
                        candidates.push_back(job_successor[curr]);
                    }
                }

                if (machine_successor[curr] != -1)
                {
                    in_degree[machine_successor[curr]]--;
                    if (in_degree[machine_successor[curr]] == 0)
                    {
                        candidates.push_back(machine_successor[curr]);
                    }
                }
                if (include_worker && worker_successor[curr] != -1)
                {
                    in_degree[worker_successor[curr]]--;
                    if (in_degree[worker_successor[curr]] == 0)
                    {
                        candidates.push_back(worker_successor[curr]);
                    }
                }
            }
        }
    }
    return result;
}

#ifdef GREEDY_INIT
void Graph::random_init(const Instance& instance, const OperationList& operation_list)
{
    const int job_num = instance.job_num;
    const int machine_num = instance.machine_num;
    this->node_num = static_cast<int>(operation_list.operations.size());

    // Initialize graph structures
    this->job_successor.assign(this->node_num, -1);
    this->machine_successor.assign(this->node_num, -1);
    this->worker_successor.assign(this->node_num, -1);
    this->job_predecessor.assign(this->node_num, -1);
    this->machine_predecessor.assign(this->node_num, -1);
    this->worker_predecessor.assign(this->node_num, -1);

    this->first_job_operation.assign(job_num, -1);
    this->last_job_operation.assign(job_num, -1);
    this->first_machine_operation.assign(machine_num, -1);
    this->last_machine_operation.assign(machine_num, -1);
    this->first_worker_operation.assign(operation_list.worker_num(), -1);
    this->last_worker_operation.assign(operation_list.worker_num(), -1);

    this->machine_operation_count.assign(machine_num, 0);
    this->worker_operation_count.assign(operation_list.worker_num(), 0);

    on_machine.resize(this->node_num, -1);
    on_worker.resize(this->node_num, -1);
    on_machine_pos_vec.resize(this->node_num, 0);
    on_worker_pos_vec.resize(this->node_num, 0);

    // ����������
    for (int job_id = 0, op_id = 1; job_id < job_num; ++job_id)
    {
        for (int op_index = 0; op_index < instance.jobs[job_id].size(); ++op_index)
        {
            if (op_index == 0)
            {
                first_job_operation[job_id] = op_id;
                job_predecessor[op_id] = 0;
                job_successor[op_id] = op_id + 1;
            }
            else if (op_index == instance.jobs[job_id].size() - 1)
            {
                last_job_operation[job_id] = op_id;
                job_predecessor[op_id] = op_id - 1;
                job_successor[op_id] = this->node_num - 1;
            }
            else
            {
                job_successor[op_id] = op_id + 1;
                job_predecessor[op_id] = op_id - 1;
            }
            ++op_id;
        }
    }

    // �޸ĺ��Giffler-Thompson�㷨
    std::vector<bool> scheduled(this->node_num, false);
    std::vector<int> machine_ready_time(machine_num, 0);
    std::vector<int> job_ready_time(job_num, 0);

    // ����ÿ̨��������Ĳ�������
    std::vector<int> machine_assigned_ops(machine_num, 0);

    // ��ʼ��ÿ��������ǰ�Ĳ���Ϊ��һ������
    std::vector<int> current_job_operation = first_job_operation;

    // �����ѵ��Ȳ���������
    int scheduled_count = 1; // ���⿪ʼ�ڵ��ѵ���

    // �����������ѡ��ķ�Χ
    constexpr double RANDOM_FACTOR = 0.2; // ��������Ž��20%�Ľ�Ҳ�л��ᱻѡ��
    constexpr double IDLE_MACHINE_BONUS = 0.2; // ��δʹ�û����Ľ�������

    // ������ѭ��
    while (scheduled_count < this->node_num - 1)
    { // ���һ������ڵ㲻��Ҫ����
        // �ҳ����пɵ��ȵĲ���
        std::vector<int> candidates;

        for (int job_id = 0; job_id < job_num; ++job_id)
        {
            int op_id = current_job_operation[job_id];
            if (op_id != -1 && op_id != this->node_num - 1 && !scheduled[op_id])
            {
                candidates.push_back(op_id);
            }
        }

        // �ҳ��ϺõĲ���-�������
        int best_completion_time = INT_MAX;
        std::vector<std::tuple<int, int, int, bool>> good_choices; // ����ID, ����ID, ���ʱ��, �Ƿ�δʹ�û���

        for (int op_id : candidates)
        {
            // ���ò������п��û���
            const auto& machine_candidates = operation_list[op_id].candidates;
            for (int machine_id : machine_candidates)
            {
                int job_id = operation_list[op_id].job_id;
                // int process_time = instance.get_process_time(job_id, operation_list[op_id].op_index, machine_id);
                int process_time = operation_list.duration(op_id, machine_id);
                int start_time = std::max(machine_ready_time[machine_id], job_ready_time[job_id]);
                int completion_time = start_time + process_time;

                // ����������ʱ��
                best_completion_time = std::min(best_completion_time, completion_time);

                // ���û����Ƿ���δ�������
                bool is_unused_machine = (machine_assigned_ops[machine_id] == 0);

                // �����������-�������
                good_choices.emplace_back(op_id, machine_id, completion_time, is_unused_machine);
            }
        }

        // ����Ƿ���δ��������Ļ���
        bool has_unused_machines = false;
        for (int m = 0; m < machine_num; m++)
        {
            if (machine_assigned_ops[m] == 0)
            {
                has_unused_machines = true;
                break;
            }
        }

        // ɸѡ"�Ϻ�"��ѡ��
        std::vector<std::tuple<int, int, int, bool>> filtered_choices;

        if (has_unused_machines)
        {
            // ���ȿ���δʹ�õĻ���
            for (const auto& choice : good_choices)
            {
                if (std::get<3>(choice))
                { // ��δʹ�õĻ���
                    // ��δʹ�û�����ѡ��ſ����ʱ������
                    int adjusted_threshold =
                        static_cast<int>(best_completion_time * (1 + RANDOM_FACTOR + IDLE_MACHINE_BONUS));
                    if (std::get<2>(choice) <= adjusted_threshold)
                    {
                        filtered_choices.push_back(choice);
                    }
                }
            }
        }

        // ���û�к��ʵ�δʹ�û���ѡ���������л���
        if (filtered_choices.empty())
        {
            int threshold = static_cast<int>(best_completion_time * (1 + RANDOM_FACTOR));
            for (const auto& choice : good_choices)
            {
                if (std::get<2>(choice) <= threshold)
                {
                    filtered_choices.push_back(choice);
                }
            }
        }

        // �ӽϺõ�ѡ�������ѡһ��
        int random_index = RAND_INT(filtered_choices.size());
        int selected_op = std::get<0>(filtered_choices[random_index]);
        int selected_machine = std::get<1>(filtered_choices[random_index]);
        const auto& worker_candidates = operation_list.workers_for_machine(selected_op, selected_machine);
        const int selected_worker = worker_candidates.empty()
            ? std::max(0, operation_list.best_worker_for_machine(selected_op, selected_machine))
            : worker_candidates[RAND_INT(static_cast<int>(worker_candidates.size()))];

        // ���µ���
        int job_id = operation_list[selected_op].job_id;
        // int process_time = instance.get_process_time(job_id, operation_list[selected_op].op_index, selected_machine);
        int process_time = operation_list.duration(selected_op, selected_machine);

        int start_time = std::max(machine_ready_time[selected_machine], job_ready_time[job_id]);
        int completion_time = start_time + process_time;

        machine_ready_time[selected_machine] = completion_time;
        job_ready_time[job_id] = completion_time;
        current_job_operation[job_id] = job_successor[selected_op];

        // ����ͼ�ṹ
        on_machine[selected_op] = selected_machine;
        on_worker[selected_op] = selected_worker;
        machine_operation_count[selected_machine]++;
        machine_assigned_ops[selected_machine]++; // ���»�������Ĳ�����

        // ���»�����
        if (first_machine_operation[selected_machine] == -1)
        {
            first_machine_operation[selected_machine] = selected_op;
            last_machine_operation[selected_machine] = selected_op;
            on_machine_pos_vec[selected_op] = 0;
        }
        else
        {
            int prev_machine_op = last_machine_operation[selected_machine];
            last_machine_operation[selected_machine] = selected_op;
            machine_successor[prev_machine_op] = selected_op;
            machine_predecessor[selected_op] = prev_machine_op;
            on_machine_pos_vec[selected_op] = machine_operation_count[selected_machine] - 1;
        }

        scheduled[selected_op] = true;
        scheduled_count++;
    }
}

#else
void Graph::random_init(const Instance& instance, const OperationList& operation_list)
{
    const int job_num = instance.job_num;
    const int machine_num = instance.machine_num;
    this->node_num = static_cast<int>(operation_list.operations.size());

    this->job_successor.assign(this->node_num, -1);
    this->machine_successor.assign(this->node_num, -1);
    this->worker_successor.assign(this->node_num, -1);
    this->job_predecessor.assign(this->node_num, -1);
    this->machine_predecessor.assign(this->node_num, -1);
    this->worker_predecessor.assign(this->node_num, -1);

    this->first_job_operation.assign(job_num, -1);
    this->last_job_operation.assign(job_num, -1);
    this->first_machine_operation.assign(machine_num, -1);
    this->last_machine_operation.assign(machine_num, -1);
    this->first_worker_operation.assign(operation_list.worker_num(), -1);
    this->last_worker_operation.assign(operation_list.worker_num(), -1);

    this->machine_operation_count.assign(machine_num, 0);
    this->worker_operation_count.assign(operation_list.worker_num(), 0);

    on_machine.resize(this->node_num, -1);
    on_worker.resize(this->node_num, -1);
    on_machine_pos_vec.resize(this->node_num, 0);
    on_worker_pos_vec.resize(this->node_num, 0);

    // ����������
    for (int job_id = 0, op_id = 1; job_id < job_num; ++job_id)
    {
        for (int op_index = 0; op_index < instance.jobs[job_id].size(); ++op_index)
        {
            if (op_index == 0)
            {
                first_job_operation[job_id] = op_id;
                job_predecessor[op_id] = 0;
                job_successor[op_id] = op_id + 1;
            }
            else if (op_index == instance.jobs[job_id].size() - 1)
            {
                last_job_operation[job_id] = op_id;
                job_predecessor[op_id] = op_id - 1;
                job_successor[op_id] = this->node_num - 1;
            }
            else
            {
                job_successor[op_id] = op_id + 1;
                job_predecessor[op_id] = op_id - 1;
            }
            ++op_id;
        }
    }

    // Keep some purely random starts; several FJSSP-W families benefit from
    // diversity more than a consistently greedy dispatching rule.
    std::vector<int> candidates = first_job_operation;
    int candidate_count = 0;
    for (int op = 1; op < this->node_num - 1; ++op)
    {
        candidate_count += static_cast<int>(operation_list[op].candidates.size());
    }
    const double relative_machine_flexibility = static_cast<double>(candidate_count) /
        std::max(1, (this->node_num - 2) * machine_num);
    const bool high_resource_parallelism = machine_num >= 50 || operation_list.worker_num() >= 50;
    const bool high_machine_flexibility = relative_machine_flexibility >= 0.3;
    const bool use_worker_aware_greedy = high_resource_parallelism || high_machine_flexibility;
    const bool use_random_construction =
        !use_worker_aware_greedy || (high_resource_parallelism && RAND_INT(100) < 35);
    if (use_random_construction)
    {
        while (!candidates.empty())
        {
            const int index = RAND_INT(static_cast<int>(candidates.size()));
            const int curr_op = candidates[index];
            const auto& curr_candidates = operation_list[curr_op].candidates;
            const int curr_machine = curr_candidates[RAND_INT(static_cast<int>(curr_candidates.size()))];
            const auto& worker_candidates = operation_list.workers_for_machine(curr_op, curr_machine);
            const int curr_worker = worker_candidates.empty()
                ? std::max(0, operation_list.best_worker_for_machine(curr_op, curr_machine))
                : worker_candidates[RAND_INT(static_cast<int>(worker_candidates.size()))];
            on_machine[curr_op] = curr_machine;
            on_worker[curr_op] = curr_worker;
            machine_operation_count[curr_machine]++;
            if (first_machine_operation[curr_machine] == -1)
            {
                first_machine_operation[curr_machine] = curr_op;
                last_machine_operation[curr_machine] = curr_op;
                on_machine_pos_vec[curr_op] = 0;
            }
            else
            {
                int pre_machine_op = last_machine_operation[curr_machine];
                last_machine_operation[curr_machine] = curr_op;
                machine_successor[pre_machine_op] = curr_op;
                machine_predecessor[curr_op] = pre_machine_op;
                on_machine_pos_vec[curr_op] = machine_operation_count[curr_machine] - 1;
            }
            if (job_successor[curr_op] == this->node_num - 1)
            {
                candidates[index] = candidates.back();
                candidates.pop_back();
            }
            else
            {
                candidates[index] = job_successor[curr_op];
            }
        }
        return;
    }

    // Build a worker-aware randomized greedy initial sequence.
    std::vector<int> machine_ready_time(machine_num, 0);
    std::vector<int> worker_ready_time(operation_list.worker_num(), 0);
    std::vector<int> job_ready_time(job_num, 0);
    std::vector<int> machine_assigned_ops(machine_num, 0);

    struct ConstructionChoice
    {
        int operation;
        int machine;
        int worker;
        int completion_time;
        bool unused_machine;
    };

    while (!candidates.empty())
    {
        std::vector<ConstructionChoice> choices;
        int best_completion_time = INT_MAX;
        bool has_unused_machine_choice = false;

        for (const int op : candidates)
        {
            if (op <= 0 || op >= this->node_num - 1)
            {
                continue;
            }

            const int job_id = operation_list[op].job_id;
            for (const int machine : operation_list[op].candidates)
            {
                const auto& worker_candidates = operation_list.workers_for_machine(op, machine);
                if (worker_candidates.empty())
                {
                    const int worker = std::max(0, operation_list.best_worker_for_machine(op, machine));
                    int duration = operation_list.worker_duration(op, machine, worker);
                    if (duration <= 0)
                    {
                        duration = operation_list.duration(op, machine);
                    }
                    int start_time = std::max(machine_ready_time[machine], job_ready_time[job_id]);
                    if (operation_list.has_workers() && worker >= 0 && worker < static_cast<int>(worker_ready_time.size()))
                    {
                        start_time = std::max(start_time, worker_ready_time[worker]);
                    }
                    const int completion_time = start_time + duration;
                    const bool unused_machine = machine_assigned_ops[machine] == 0;
                    choices.push_back({ op, machine, worker, completion_time, unused_machine });
                    best_completion_time = std::min(best_completion_time, completion_time);
                    has_unused_machine_choice = has_unused_machine_choice || unused_machine;
                    continue;
                }

                for (const int worker : worker_candidates)
                {
                    int duration = operation_list.worker_duration(op, machine, worker);
                    if (duration <= 0)
                    {
                        duration = operation_list.duration(op, machine);
                    }
                    int start_time = std::max(machine_ready_time[machine], job_ready_time[job_id]);
                    if (operation_list.has_workers() && worker >= 0 && worker < static_cast<int>(worker_ready_time.size()))
                    {
                        start_time = std::max(start_time, worker_ready_time[worker]);
                    }
                    const int completion_time = start_time + duration;
                    const bool unused_machine = machine_assigned_ops[machine] == 0;
                    choices.push_back({ op, machine, worker, completion_time, unused_machine });
                    best_completion_time = std::min(best_completion_time, completion_time);
                    has_unused_machine_choice = has_unused_machine_choice || unused_machine;
                }
            }
        }

        if (choices.empty())
        {
            break;
        }

        std::vector<ConstructionChoice> filtered_choices;
        const int threshold = best_completion_time + std::max(1, best_completion_time / 5);
        if (has_unused_machine_choice)
        {
            for (const auto& choice : choices)
            {
                if (choice.unused_machine && choice.completion_time <= threshold)
                {
                    filtered_choices.push_back(choice);
                }
            }
        }
        if (filtered_choices.empty())
        {
            for (const auto& choice : choices)
            {
                if (choice.completion_time <= threshold)
                {
                    filtered_choices.push_back(choice);
                }
            }
        }
        if (filtered_choices.empty())
        {
            filtered_choices = std::move(choices);
        }

        const auto selected_choice = filtered_choices[RAND_INT(static_cast<int>(filtered_choices.size()))];
        const int curr_op = selected_choice.operation;
        const int curr_machine = selected_choice.machine;
        const int curr_worker = selected_choice.worker;
        on_machine[curr_op] = curr_machine;
        on_worker[curr_op] = curr_worker;
        machine_operation_count[curr_machine]++;
        machine_ready_time[curr_machine] = selected_choice.completion_time;
        job_ready_time[operation_list[curr_op].job_id] = selected_choice.completion_time;
        if (curr_worker >= 0 && curr_worker < static_cast<int>(worker_ready_time.size()))
        {
            worker_ready_time[curr_worker] = selected_choice.completion_time;
        }
        // �����ǰ�����ǻ����ϵĵ�һ��������������� first_machine_operation
        if (first_machine_operation[curr_machine] == -1)
        {
            first_machine_operation[curr_machine] = curr_op;
            last_machine_operation[curr_machine] = curr_op;
            // ����on_machine_pos = 0
            on_machine_pos_vec[curr_op] = 0;
        }
        else
        {
            int pre_machine_op = last_machine_operation[curr_machine];
            last_machine_operation[curr_machine] = curr_op;
            machine_successor[pre_machine_op] = curr_op;
            machine_predecessor[curr_op] = pre_machine_op;
            // ����on_machine_posΪ��ǰ��������������һ����0��ʼ��
            on_machine_pos_vec[curr_op] = machine_operation_count[curr_machine] - 1;
        }
        machine_assigned_ops[curr_machine]++;
        // �����ǰ�����ǹ��������һ������,����Ӻ�ѡ�����������Ƴ�
        const auto index_it = std::find(candidates.begin(), candidates.end(), curr_op);
        if (index_it == candidates.end())
        {
            continue;
        }
        const int index = static_cast<int>(index_it - candidates.begin());
        if (job_successor[curr_op] == this->node_num - 1)
        {
            candidates[index] = candidates.back();
            candidates.pop_back();
        }
        else
        {
            candidates[index] = job_successor[curr_op];
        }
    }
}
#endif

void Graph::make_move(const NeighborhoodMove& move)
{
    if (move.method == Method::CHANGE_WORKER)
    {
        if (move.which > 0 && move.which < node_num - 1)
        {
            on_worker[move.which] = move.where;
        }
        return;
    }

    auto is_reachable_forward = [&](int from, int target) -> bool
    {
        int guard = 0;
        for (int op = from; op != -1 && guard <= node_num; op = machine_successor[op], ++guard)
        {
            if (op == target)
            {
                return true;
            }
        }
        return false;
    };

    auto is_reachable_backward = [&](int from, int target) -> bool
    {
        int guard = 0;
        for (int op = from; op != -1 && guard <= node_num; op = machine_predecessor[op], ++guard)
        {
            if (op == target)
            {
                return true;
            }
        }
        return false;
    };

    // Update disjunctive graph edges based on move direction
    if (move.method == Method::FRONT)
    {
        if (move.which <= 0 || move.which >= node_num - 1 || move.where <= 0 || move.where >= node_num - 1)
        {
            return;
        }
        if (on_machine[move.which] != on_machine[move.where])
        {
            return;
        }
        if (!is_reachable_forward(move.where, move.which))
        {
            return;
        }

        // Forward move: swap operation 'which' before 'where'
        const int ms_u = this->machine_successor[move.which];
        const int mp_u = this->machine_predecessor[move.which];
        const int mp_v = this->machine_predecessor[move.where];

        // �� vBu �� u �ƶ��� v ��ǰ��
        // vB ��λ�ü�1
        // uΪԭ��v��λ��
        on_machine_pos_vec[move.which] = on_machine_pos_vec[move.where];
        int guard = 0;
        for (auto op = move.where; op != move.which && op != -1 && guard <= node_num; op = this->machine_successor[op], ++guard)
        {
            on_machine_pos_vec[op]++;
        }

        // Update machine edges
        this->machine_successor[move.which] = move.where;
        this->machine_predecessor[move.where] = move.which;

        if (mp_v != -1)
        {
            this->machine_successor[mp_v] = move.which;
        }
        else
        {
            // Update first operation on machine if needed
            // auto it = std::ranges::find(this->first_machine_operation, move.where);
            // if (it != this->first_machine_operation.end())
            // {
            //     *it = move.which;
            // }
            // first operation on machine �� move.where ��Ϊ move.which
            first_machine_operation[on_machine[move.where]] = move.which;
        }

        this->machine_predecessor[move.which] = mp_v;

        if (ms_u != -1)
        {
            this->machine_predecessor[ms_u] = mp_u;
        }
        else
        {
            // Update last operation on machine if needed
            // auto it = std::ranges::find(this->last_machine_operation, move.which);
            // if (it != this->last_machine_operation.end())
            // {
            //     *it = mp_u;
            // }
            last_machine_operation[on_machine[move.which]] = mp_u;
        }

        if (mp_u != -1)
        {
            this->machine_successor[mp_u] = ms_u;
        }
    }
    else if (move.method == Method::BACK)
    {
        if (move.which <= 0 || move.which >= node_num - 1 || move.where <= 0 || move.where >= node_num - 1)
        {
            return;
        }
        if (on_machine[move.which] != on_machine[move.where])
        {
            return;
        }
        if (!is_reachable_backward(move.where, move.which))
        {
            return;
        }

        // Backward move: swap operation 'which' after 'where'
        const int ms_u = this->machine_successor[move.which];
        const int ms_v = this->machine_successor[move.where];
        const int mp_u = this->machine_predecessor[move.which];

        // �� u �ƶ��� v �ĺ���
        // ԭ��uBv���У�Bv��pos��1��u��λ��Ϊv��pos
        on_machine_pos_vec[move.which] = on_machine_pos_vec[move.where];
        int guard = 0;
        for (auto op = move.where; op != move.which && op != -1 && guard <= node_num; op = machine_predecessor[op], ++guard)
        {
            on_machine_pos_vec[op]--;
        }

        // Update machine edges
        this->machine_successor[move.where] = move.which;
        this->machine_predecessor[move.which] = move.where;

        if (mp_u != -1)
        {
            this->machine_successor[mp_u] = ms_u;
        }
        else
        {
            // Update first operation on machine if needed
            auto it = std::ranges::find(this->first_machine_operation, move.which);
            if (it != this->first_machine_operation.end())
            {
                *it = ms_u;
            }
        }

        if (ms_u != -1)
        {
            this->machine_predecessor[ms_u] = mp_u;
        }

        if (ms_v != -1)
        {
            this->machine_predecessor[ms_v] = move.which;
        }
        else
        {
            // Update last operation on machine if needed
            auto it = std::ranges::find(this->last_machine_operation, move.where);
            if (it != this->last_machine_operation.end())
            {
                *it = move.which;
            }
        }

        this->machine_successor[move.which] = ms_v;
    }
    else
    {
        // ��������������
        int u = 0, v = 0;
        const auto old_machine = on_machine[move.which];
        const auto new_machine = on_machine[move.where];

        // ���»����ϵĹ�������
        machine_operation_count[new_machine]++;
        machine_operation_count[old_machine]--;


        // �� move.which ���뵽 u �� v �м�
        if (move.method == Method::CHANGE_MACHINE_BACK)
        {
            u = move.where;
            v = machine_successor[u];

            // ���� which �Ļ���λ��
            on_machine_pos_vec[move.which] = on_machine_pos_vec[u] + 1;
        }
        else
        {
            v = move.where;
            u = machine_predecessor[v];

            // ���� which �Ļ���λ��
            on_machine_pos_vec[move.which] = on_machine_pos_vec[v];
        }

        // ��ԭ����move.whichͬ��������Ĳ�����λ�ü�1
        for (auto op = machine_successor[move.which]; op != -1; op = machine_successor[op])
        {
            on_machine_pos_vec[op]--;
        }


        // �� v ����Ĳ�����λ�ü�1
        for (auto op = v; op != -1; op = machine_successor[op])
        {
            on_machine_pos_vec[op]++;
        }

        on_machine[move.which] = new_machine;


        // ����ԭ�������Ļ�����
        const auto old_mp_which = machine_predecessor[move.which];
        const auto old_ms_which = machine_successor[move.which];
        // ���ԭ���Ļ����� move.which ���ǵ�һ������
        if (old_mp_which != -1)
        {
            machine_successor[old_mp_which] = old_ms_which;
        }
        else
        {
            first_machine_operation[old_machine] = old_ms_which;
        }
        // ���ԭ���Ļ����� move.which �������һ������
        if (old_ms_which != -1)
        {
            machine_predecessor[old_ms_which] = old_mp_which;
        }
        else
        {
            // ���򽫻��������һ��������Ϊ move.which ����һ������
            last_machine_operation[old_machine] = old_mp_which;
        }

        // �����»����Ļ�����
        if (u != -1)
        {
            machine_successor[u] = move.which;
        }
        else
        {
            first_machine_operation[new_machine] = move.which;
        }
        machine_predecessor[move.which] = u;
        machine_successor[move.which] = v;
        if (v != -1)
        {
            machine_predecessor[v] = move.which;
        }
        else
        {
            last_machine_operation[new_machine] = move.which;
        }
    }
}
[[maybe_unused]] int Graph::on_machine_pos(const int op_id) const
{
    int pos = 0;
    const int machine_id = on_machine[op_id];
    for (int op = first_machine_operation[machine_id]; op != op_id; op = machine_successor[op])
    {
        pos++;
    }
    return pos;
}

//int Graph::distance(const Graph& other) const
//{
//    int result = 0;
//    for (int op = 1; op < node_num - 1; op++)
//    {
//        const int pos1 = this->on_machine_pos_vec[op];
//        const int pos2 = other.on_machine_pos_vec[op];
//        const int machine1 = this->on_machine[op];
//        // ����������ͬһ�������ڲ�ͬ�Ļ�����
//        if (const int machine2 = other.on_machine[op]; machine1 != machine2)
//        {
//            int sum = pos1 + pos2;
//            result +=
//                std::min(sum, this->machine_operation_count[machine1] + other.machine_operation_count[machine2] - sum);
//        }
//        else
//        {
//            result += std::abs(pos1 - pos2);
//        }
//    }
//    return result;
//}
