//
// Created by qiming on 25-4-13.
//
#include "Schedule.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <iostream>
#include <queue>
#include <ranges>
#include <unordered_set>
#include <numeric>
#include <random>

#include "RandomGenerator.h"

#define NEXT_MACHINE_OP(op) graph.machine_successor[op]
#define NEXT_JOB_OP(op) graph.job_successor[op]
#define PREV_JOB_OP(op) graph.job_predecessor[op]


void Schedule::update_time()
{
    int n = graph.node_num;
    time_info.resize(n);
    assigned_worker.assign(n, -1);
    assigned_duration.assign(n, 0);

    auto forward_queue = graph.topological_sort(false);
    makespan = 0;
    std::vector<int> worker_ready;
    if (operation_list->has_workers())
    {
        worker_ready.assign(operation_list->worker_num(), 0);
    }

    for (int i = 1; i < n - 1; i++)
    {
        int curr_node = forward_queue[i];
        int start_time = 0;
        int prev_op_id = graph.job_predecessor[curr_node];
        int prev_machine_id = graph.machine_predecessor[curr_node];
        const int machine = graph.on_machine[curr_node];

        if (prev_op_id != -1)
        {
            start_time = time_info[prev_op_id].end_time;
        }

        if (prev_machine_id != -1)
        {
            start_time = std::max(start_time, time_info[prev_machine_id].end_time);
        }

        int chosen_worker = operation_list->best_worker_for_machine(curr_node, machine);
        int duration = operation_list->duration(curr_node, machine);
        int operation_start = start_time;

        if (operation_list->has_workers())
        {
            const auto& workers = operation_list->workers_for_machine(curr_node, machine);
            int best_end = INT_MAX;
            int best_start = INT_MAX;
            int best_worker = -1;
            int best_duration = duration;

            for (const int worker : workers)
            {
                const int worker_duration = operation_list->worker_duration(curr_node, machine, worker);
                if (worker_duration <= 0)
                {
                    continue;
                }
                const int candidate_start = std::max(start_time, worker_ready[worker]);
                const int candidate_end = candidate_start + worker_duration;
                if (candidate_end < best_end || (candidate_end == best_end && candidate_start < best_start))
                {
                    best_end = candidate_end;
                    best_start = candidate_start;
                    best_worker = worker;
                    best_duration = worker_duration;
                }
            }

            if (best_worker != -1)
            {
                chosen_worker = best_worker;
                duration = best_duration;
                operation_start = best_start;
                worker_ready[best_worker] = best_end;
            }
        }

        time_info[curr_node].operator_id = curr_node;
        time_info[curr_node].forward_path_length = operation_start;
        const int end_time = operation_start + duration;
        assigned_worker[curr_node] = chosen_worker;
        assigned_duration[curr_node] = duration;

        if (end_time > makespan)
        {
            makespan = end_time;
        }
        time_info[curr_node].end_time = end_time;
    }


    for (int i = n - 2; i > 0; --i)
    {
        int op = forward_queue[i];
        int js = graph.job_successor[op];
        int ms = graph.machine_successor[op];
        int q = 0;
        if (js != n - 1)
        {
            q = time_info[js].backward_path_length + assigned_duration[js];
        }
        if (ms != -1)
        {
            q = std::max(q, time_info[ms].backward_path_length + assigned_duration[ms]);
        }
        time_info[op].backward_path_length = q;
    }
}


[[maybe_unused]] void Schedule::export_schedule(const char* filename) const
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + std::string(filename));
    }
    file << "ID,Job,Operation,Machine,Worker,Duration,StartTime,EndTime,IsCritical" << std::endl;
    for (int i = 1; i < graph.node_num - 1; i++)
    {
        file << i << "," << (*operation_list)[i].job_id << "," << (*operation_list)[i].index << ","
            << graph.on_machine[i] << "," << assigned_worker[i] << "," << assigned_duration[i] << ","
            << time_info[i].forward_path_length << "," << time_info[i].end_time << ","
            << is_critical_operation(i) << std::endl;
    }

    file.close();
    std::clog << "Schedule exported to " << filename << std::endl;
}

#ifdef LEGAL_ZHANGCHAOYONG
bool Schedule::is_legal_move(const NeighborhoodMove& move) const
{
    bool result = true;
    if (move.which <= 0 || move.which >= graph.node_num - 1 ||
        move.where <= 0 || move.where >= graph.node_num - 1)
        return false;
    if (move.which == move.where)
        return false;
    if (move.method == Method::FRONT && graph.machine_successor[move.which] == move.where)
        return false;
    if (move.method == Method::BACK && graph.machine_predecessor[move.which] == move.where)
        return false;
    if (move.method == Method::BACK)
    {
        auto js_u = NEXT_JOB_OP(move.which);
        result = time_info[move.where].backward_path_length +
            operation_list->duration(move.where, graph.on_machine[move.where]) >
            time_info[js_u].backward_path_length + operation_list->duration(js_u, graph.on_machine[js_u]);
    }
    if (move.method == Method::FRONT)
    {
        result = time_info[move.where].end_time > time_info[PREV_JOB_OP(move.which)].end_time;
    }
#ifdef UNIT_TEST
    if (move.method == Method::BACK || move.method == Method::FRONT)
    {
        bool right = true;
        try
        {
            auto tmp = *this;
            tmp.make_move(move);
        }
        catch (const std::exception& e)
        {
            right = false;
        }
        static int count = 0;
        static int count_right = 0;
        if (result == right)
            count_right += 1;
        count += 1;
        if (count % 100000 == 0)
        {

            std::clog << static_cast<double>(count_right) / static_cast<double>(count) * 100 << "%" << std::endl;
            count = 0;
            count_right = 0;
        }
    }
#endif
    return result;
}
#else
bool Schedule::is_legal_move(const NeighborhoodMove& move) const
{
    bool result = true;
    if (move.which <= 0 || move.which >= graph.node_num - 1 ||
        move.where <= 0 || move.where >= graph.node_num - 1)
        return false;
    if (move.which == move.where)
        return false;
    if (move.method == Method::FRONT && graph.machine_successor[move.which] == move.where)
        return false;
    if (move.method == Method::BACK && graph.machine_predecessor[move.which] == move.where)
        return false;
    if (move.method == Method::BACK)
    {
        auto js_u = NEXT_JOB_OP(move.which);
        if (js_u == move.where)
            result = false;
        else
        {
            result = time_info[move.where].backward_path_length +
                operation_list->duration(move.where, graph.on_machine[move.where]) >=
                time_info[js_u].backward_path_length + operation_list->duration(js_u, graph.on_machine[js_u]);
        }
    }
    if (move.method == Method::FRONT)
    {
        if (move.where == PREV_JOB_OP(move.which))
            result = false;
        else
            result = time_info[move.where].end_time >= time_info[PREV_JOB_OP(move.which)].end_time;
    }
#ifdef UNIT_TEST
    if (move.method == Method::BACK || move.method == Method::FRONT)
    {
        bool right = true;
        try
        {
            auto tmp = *this;
            tmp.make_move(move);
        }
        catch (const std::exception& e)
        {
            right = false;
        }
        static int count = 0;
        static int count_right = 0;
        if (result == right)
            count_right += 1;
        count += 1;
        if (count % 100000 == 0)
        {

            std::clog << static_cast<double>(count_right) / static_cast<double>(count) * 100 << "%" << std::endl;
        }
    }
#endif
    return result;
}
#endif
void Schedule::make_move(const NeighborhoodMove& move)
{
    graph.make_move(move);
    update_time();
}


//int Schedule::distance(const Schedule& other) const { return this->graph.distance(other.graph); }

void Schedule::output() const
{
    std::cout << "makespan=" << makespan << std::endl;
    for (int i = 0; i < graph.machine_operation_count.size(); i++)
    {
        int machine_op_num = graph.machine_operation_count[i];
        std::cout << machine_op_num << "\t";
        for (auto op = graph.first_machine_operation[i]; op != -1; op = graph.machine_successor[op])
        {
            const auto& op_info = operation_list->operations[op];
            std::cout << op_info.job_id << " " << op_info.index << " w=" << assigned_worker[op] << "\t";
        }
        std::cout << std::endl;
    }
}
//int delta_distance(const Schedule& current, const Schedule& guiding, const NeighborhoodMove& move)
//{
//    const auto& current_graph = current.graph;
//    const auto& guiding_graph = guiding.graph;
//
//    // ����������������������֮��ľ���
//    auto calculate_op_distance = [&](int op, int pos1, int machine1, int pos2, int machine2) -> int
//    {
//        if (machine1 != machine2)
//        {
//            int sum_pos = pos1 + pos2;
//            int sum_machines =
//                current_graph.machine_operation_count[machine1] + guiding_graph.machine_operation_count[machine2];
//            return std::min(sum_pos, sum_machines - sum_pos);
//        }
//        else
//        {
//            return std::abs(pos1 - pos2);
//        }
//    };
//
//    // ����ͬһ�����ϵ������ƶ�
//    if (move.method == Method::BACK || move.method == Method::FRONT)
//    {
//        const auto machine_id = current_graph.on_machine[move.where];
//        int old_affected_distance = 0;
//        int new_affected_distance = 0;
//
//        // ȷ�������ķ�Χ��λ�ñ仯����
//        int start_op, end_check;
//        int pos_modifier;
//        if (move.method == Method::BACK)
//        {
//            start_op = move.which;
//            end_check = current_graph.machine_successor[move.where];
//            pos_modifier = -1;
//        }
//        else
//        { // FRONT
//            start_op = move.where;
//            end_check = current_graph.machine_successor[move.which];
//            pos_modifier = 1;
//        }
//
//        // ����Ӱ��Ĳ����������仯
//        for (auto op = start_op; op != end_check; op = current_graph.machine_successor[op])
//        {
//            const int pos1_old = current_graph.on_machine_pos_vec[op];
//            const int pos1_new =
//                (op == move.which) ? current_graph.on_machine_pos_vec[move.where] : pos1_old + pos_modifier;
//            const int pos2 = guiding_graph.on_machine_pos_vec[op];
//            const int machine2 = guiding_graph.on_machine[op];
//
//            old_affected_distance += calculate_op_distance(op, pos1_old, machine_id, pos2, machine2);
//            new_affected_distance += calculate_op_distance(op, pos1_new, machine_id, pos2, machine2);
//        }
//
//        return new_affected_distance - old_affected_distance;
//    }
//    else // �������������������
//    {
//        int k = move.which;
//        int u, v;
//        if (move.method == Method::CHANGE_MACHINE_BACK)
//        {
//            u = move.where;
//            v = current_graph.machine_successor[u];
//        }
//        else
//        {
//            v = move.where;
//            u = current_graph.machine_predecessor[v];
//        }
//
//        const int old_machine = current_graph.on_machine[k];
//        const int new_machine = current_graph.on_machine[move.where];
//        const int old_machine_count = current_graph.machine_operation_count[old_machine];
//        const int new_machine_count = current_graph.machine_operation_count[new_machine];
//        const int guide_pos_k = guiding_graph.on_machine_pos_vec[k];
//
//        int old_affected_distance = 0;
//        int new_affected_distance = 0;
//
//        // 1. ����k��������в���
//        for (auto op = current_graph.machine_successor[k]; op != -1; op = current_graph.machine_successor[op])
//        {
//            const int pos1_old = current_graph.on_machine_pos_vec[op];
//            const int pos1_new = pos1_old - 1;
//            const int pos2 = guiding_graph.on_machine_pos_vec[op];
//            const int machine2 = guiding_graph.on_machine[op];
//
//            old_affected_distance += calculate_op_distance(op, pos1_old, old_machine, pos2, machine2);
//            // ����k���ߺ�����ϵĲ�������1
//            if (old_machine != machine2)
//            {
//                int sum_new = pos1_new + pos2;
//                int sum_machines = old_machine_count - 1 + guiding_graph.machine_operation_count[machine2];
//                new_affected_distance += std::min(sum_new, sum_machines - sum_new);
//            }
//            else
//            {
//                new_affected_distance += std::abs(pos1_new - pos2);
//            }
//        }
//
//        // 2. ����kǰ������в���
//        for (auto op = current_graph.machine_predecessor[k]; op != -1; op = current_graph.machine_predecessor[op])
//        {
//            const int machine2 = guiding_graph.on_machine[op];
//            if (machine2 != old_machine)
//            {
//                const int pos1 = current_graph.on_machine_pos_vec[op];
//                const int pos2 = guiding_graph.on_machine_pos_vec[op];
//
//                old_affected_distance += calculate_op_distance(op, pos1, old_machine, pos2, machine2);
//                // ����k���ߺ�����ϵĲ�������1
//                int sum_pos = pos1 + pos2;
//                int sum_machines_new = old_machine_count - 1 + guiding_graph.machine_operation_count[machine2];
//                new_affected_distance += std::min(sum_pos, sum_machines_new - sum_pos);
//            }
//        }
//
//        // 3. ����v��������в���
//        for (auto op = v; op != -1; op = current_graph.machine_successor[op])
//        {
//            const int pos1_old = current_graph.on_machine_pos_vec[op];
//            const int pos1_new = pos1_old + 1;
//            const int pos2 = guiding_graph.on_machine_pos_vec[op];
//            const int machine2 = guiding_graph.on_machine[op];
//
//            old_affected_distance += calculate_op_distance(op, pos1_old, new_machine, pos2, machine2);
//            // ����k���������ϵĲ�������1
//            if (new_machine != machine2)
//            {
//                int sum_new = pos1_new + pos2;
//                int sum_machines = new_machine_count + 1 + guiding_graph.machine_operation_count[machine2];
//                new_affected_distance += std::min(sum_new, sum_machines - sum_new);
//            }
//            else
//            {
//                new_affected_distance += std::abs(pos1_new - pos2);
//            }
//        }
//
//        // 4. ����uǰ������в���
//        for (auto op = u; op != -1; op = current_graph.machine_predecessor[op])
//        {
//            const int machine2 = guiding_graph.on_machine[op];
//            if (machine2 != new_machine)
//            {
//                const int pos1 = current_graph.on_machine_pos_vec[op];
//                const int pos2 = guiding_graph.on_machine_pos_vec[op];
//
//                old_affected_distance += calculate_op_distance(op, pos1, new_machine, pos2, machine2);
//                // ����k���������ϵĲ�������1
//                int sum_pos = pos1 + pos2;
//                int sum_machines_new = new_machine_count + 1 + guiding_graph.machine_operation_count[machine2];
//                new_affected_distance += std::min(sum_pos, sum_machines_new - sum_pos);
//            }
//        }
//
//        // 5. ����k����
//        const int old_pos_k = current_graph.on_machine_pos_vec[k];
//        const int new_pos_k = v == -1 ? current_graph.on_machine_pos_vec[u] + 1 : current_graph.on_machine_pos_vec[v];
//
//        // k��ԭ��������guiding��ͬ����
//        int sum_old = old_pos_k + guide_pos_k;
//        int sum_machines_old = old_machine_count + guiding_graph.machine_operation_count[new_machine];
//        old_affected_distance += std::min(sum_old, sum_machines_old - sum_old);
//
//        // k�Ƶ���guiding��ͬ�Ļ���
//        new_affected_distance += std::abs(new_pos_k - guide_pos_k);
//
//        return new_affected_distance - old_affected_distance;
//    }
//}



// zi����
int operation_zi(const std::vector<int>& w, const std::vector<int>& t, std::mt19937& rng) {
    std::vector<double> zi_values(w.size());
    //std::unordered_map<int, double> zi_map;
    // ����ÿ��������ziֵ
    
    for (int id = 0; id < (int)w.size(); ++id) {
        std::uniform_real_distribution<double> rand_d(0.0, 1.0);
        double rr = rand_d(rng); // �����
        //double zi = std::max(0.0, (1.0 - (double)t[id] / (double)rr)) * (double)w[id];
        zi_values[id] = std::max(0.0, (1.0 - (double)t[id] / (double)rr)) * (double)w[id];
    }
    return 0;
}

//Schedule path_relinking(const Schedule& initial, const Schedule& guiding)
//{
//    Schedule current = initial;
//    int distance_initial_to_guiding = guiding.distance(initial);
//    constexpr double alpha = 0.4;
//    std::vector<Graph> PathSet;
//    std::vector<int> PathSet_makespan;
//    const int node_num = current.graph.node_num;
//    int distance_current_to_guiding = current.distance(guiding);
//    int times = 0;
//    while (distance_current_to_guiding > alpha * distance_initial_to_guiding)
//    {
//        // �������ƶȹ���
//        if (++times == 5000)
//        {
//            return current;
//        }
//        constexpr int gamma = 5;
//        // ���ڼ�¼ÿ�����������Ŷ���
//        std::vector<NeighborhoodMove> all_moves;
//        // ��¼��Ӧ��distance
//        std::vector<int> all_moves_distance;
//        // ��¼��Ӧ��makespan
//        std::vector<int> all_moves_makespan;
//        all_moves.reserve(node_num);
//        all_moves_distance.reserve(node_num);
//        all_moves_makespan.reserve(node_num);
//
//        // ����sc�е�ÿһ���������������˳��
//        std::vector<int> op_vec(current.graph.node_num - 2);
//        std::iota(op_vec.begin(), op_vec.end(), 1); // ���Ϊ 1 �� n
//        std::ranges::shuffle(op_vec, RANDOM_GENERATOR.getGen()); // ϴ���㷨
//        // �������в���
//        for (const auto op : op_vec)
//        {
//            // ��ǰ������sc���ĸ�������
//            const int op_sc_machine = current.graph.on_machine[op];
//
//            std::vector<NeighborhoodMove> best_moves;
//            int min_distance = INT_MAX;
//
//            // ����ò����Ļ�����ֻ��һ��������������
//            if (current.graph.machine_operation_count[op_sc_machine] <= 1)
//                continue;
//            // ����ò����Ļ������� guiding_schedule �Ļ��������Խ�������
//
//            std::vector<int> intersection;
//            // ��op������guiding��op�Ļ�����
//            if (op_sc_machine != guiding.graph.on_machine[op])
//            {
//                std::vector<NeighborhoodMove> op_moves;
//                const auto new_machine = guiding.graph.on_machine[op];
//
//                std::vector<int> RK;
//                std::vector<int> LK;
//                const auto job_pre = current.graph.job_predecessor[op];
//                const auto job_suc = current.graph.job_successor[op];
//
//                const int remove_machine_R_op = current.time_info[job_pre].end_time;
//                int remove_machine_Q_op = 0;
//                if (job_suc != current.graph.node_num - 1)
//                {
//                    remove_machine_Q_op = current.time_info[job_suc].backward_path_length +
//                        (*current.operation_list)[job_suc].duration.at(current.graph.on_machine[job_suc]);
//                }
//                const int first_machine_op = current.graph.first_machine_operation[new_machine];
//                for (auto u = first_machine_op; u != -1; u = current.graph.machine_successor[u])
//                {
//                    int t1 = current.time_info[u].end_time;
//                    int t2 = current.time_info[u].backward_path_length +
//                        (*current.operation_list)[u].duration.at(new_machine);
//                    if (t1 > remove_machine_R_op)
//                    {
//                        RK.push_back(u);
//                    }
//
//                    if (t2 > remove_machine_Q_op)
//                    {
//                        LK.push_back(u);
//                    }
//                }
//
//                intersection.reserve(std::min(RK.size(), LK.size()));
//                std::unordered_set<int> RK_SET(RK.cbegin(), RK.cend());
//                for (const auto op_LK : LK)
//                {
//                    if (RK_SET.contains(op_LK))
//                    {
//                        intersection.push_back(op_LK);
//                    }
//                }
//
//                if (!intersection.empty())
//                {
//                    op_moves.emplace_back(Method::CHANGE_MACHINE_FRONT, op, intersection.front());
//                    for (const auto u : intersection)
//                    {
//                        op_moves.emplace_back(Method::CHANGE_MACHINE_BACK, op, u);
//                    }
//                }
//                else
//                {
//                    if (!LK.empty() && !RK.empty())
//                    {
//                        for (int start = LK.back(); start != RK.front(); start = current.graph.machine_successor[start])
//                        {
//                            op_moves.emplace_back(Method::CHANGE_MACHINE_BACK, op, start);
//                        }
//                    }
//                    else if (LK.empty() && !RK.empty())
//                    {
//                        op_moves.emplace_back(Method::CHANGE_MACHINE_FRONT, op, RK.front());
//                    }
//                    else if (!LK.empty() && RK.empty())
//                    {
//                        op_moves.emplace_back(Method::CHANGE_MACHINE_BACK, op, LK.back());
//                    }
//                }
//#ifdef UNIT_TEST
//                std::vector<NeighborhoodMove> op_moves_2;
//                auto start_test_op = current.graph.first_machine_operation[new_machine];
//                NeighborhoodMove move_1(Method::CHANGE_MACHINE_FRONT, op, start_test_op);
//                bool legal1 = true;
//                try
//                {
//                    auto tmp = current;
//                    tmp.make_move(move_1);
//                }
//                catch (const std::exception& e)
//                {
//                    legal1 = false;
//                }
//                if (legal1)
//                {
//                    op_moves_2.push_back(move_1);
//                }
//                for (auto test_op = start_test_op; test_op != -1; test_op = current.graph.machine_successor[test_op])
//                {
//                    NeighborhoodMove move_2(Method::CHANGE_MACHINE_BACK, op, test_op);
//                    bool legal = true;
//                    try
//                    {
//                        auto tmp = current;
//                        tmp.make_move(move_2);
//                    }
//                    catch (const std::exception& e)
//                    {
//                        legal = false;
//                    }
//                    if (legal)
//                    {
//                        op_moves_2.push_back(move_2);
//                    }
//                }
//                static int all_times = 0;
//                static int right_times = 0;
//                all_times++;
//                if (op_moves.size() == op_moves_2.size())
//                {
//                    right_times++;
//                }
//                if (all_times % 1000 == 0)
//                {
//                    std::cout << static_cast<double>(right_times) / all_times << std::endl;
//                }
//#endif
//                // �������е�op_moves
//                for (const auto& move : op_moves)
//                {
//                    int distance = distance_current_to_guiding + delta_distance(current, guiding, move);
//                    if (distance < min_distance)
//                    {
//                        min_distance = distance;
//                        best_moves.clear();
//                        best_moves.push_back(move);
//                    }
//                    else if (distance == min_distance)
//                    {
//                        best_moves.push_back(move);
//                    }
//                }
//            }
//            else
//            {
//                if (current.graph.on_machine_pos_vec[op] == guiding.graph.on_machine_pos_vec[op])
//                    continue;
//                // �������в��ı������������
//                for (auto v = current.graph.machine_predecessor[op]; v != -1; v = current.graph.machine_predecessor[v])
//                {
//                    if (NeighborhoodMove move{ Method::FRONT, op, v }; current.is_legal_move(move))
//                    {
//                        int distance = distance_current_to_guiding + delta_distance(current, guiding, move);
//                        if (distance < min_distance)
//                        {
//                            min_distance = distance;
//                            best_moves.clear();
//                            best_moves.push_back(move);
//                        }
//                        else if (distance == min_distance)
//                        {
//                            best_moves.push_back(move);
//                        }
//                    }
//                }
//                for (auto u = current.graph.machine_successor[op]; u != -1; u = current.graph.machine_successor[u])
//                {
//                    if (NeighborhoodMove move{ Method::BACK, op, u }; current.is_legal_move(move))
//                    {
//                        int distance = distance_current_to_guiding + delta_distance(current, guiding, move);
//                        if (distance < min_distance)
//                        {
//                            min_distance = distance;
//                            best_moves.clear();
//                            best_moves.push_back(move);
//                        }
//                        else if (distance == min_distance)
//                        {
//                            best_moves.push_back(move);
//                        }
//                    }
//                }
//            }
//
//            if (min_distance <= distance_initial_to_guiding)
//            {
//                NeighborhoodMove selected_move = best_moves[RAND_INT(best_moves.size())];
//                all_moves.push_back(selected_move);
//                all_moves_distance.push_back(min_distance);
//                if (selected_move.method == Method::BACK || selected_move.method == Method::FRONT)
//                {
//                    all_moves_makespan.push_back(same_machine_evaluate(current, selected_move));
//                }
//                else
//                {
//                    all_moves_makespan.push_back(change_machine_evaluate(current, selected_move, intersection));
//                }
//            }
//        }
//
//
//        int moves_num = static_cast<int>(all_moves.size());
//        if (moves_num == 0)
//        {
//            initial.output();
//            std::clog << "No legal move found!" << std::endl;
//            std::clog << "Final solution: " << initial.get_makespan() << std::endl;
//            exit(0);
//        }
//        // idx �����ݴ��±���Ϣ��rank_distance��������������rank_makespan����makespan������
//        std::vector<int> idx(moves_num), rank_distance(moves_num), rank_makespan(moves_num);
//        // �ȼ���distance����
//        for (int i = 0; i < moves_num; ++i)
//            idx[i] = i;
//        std::ranges::sort(idx, [&](const int a, const int b) { return all_moves_distance[a] < all_moves_distance[b]; });
//        for (int i = 0; i < moves_num; ++i)
//        {
//            rank_distance[idx[i]] = i + 1; // 1-based
//        }
//        // �ټ���makespan����
//        for (int i = 0; i < moves_num; ++i)
//            idx[i] = i;
//        std::ranges::sort(idx, [&](const int a, const int b) { return all_moves_makespan[a] < all_moves_makespan[b]; });
//        for (int i = 0; i < moves_num; ++i)
//        {
//            rank_makespan[idx[i]] = i + 1; // 1-based
//        }
//
//        // ���������
//        for (int i = 0; i < moves_num; ++i)
//        {
//            rank_distance[i] += rank_makespan[i];
//        }
//        // ȡ��������ǰk��(gamma��)
//        int k = std::min(moves_num, gamma);
//        for (int i = 0; i < moves_num; ++i)
//            idx[i] = i;
//        std::ranges::partial_sort(idx, idx.begin() + k,
//            [&](const int a, const int b) { return rank_distance[a] < rank_distance[b]; });
//        std::vector top_k(idx.begin(), idx.begin() + k);
//        int select_index = top_k[RAND_INT(k)];
//        NeighborhoodMove best_move = all_moves[select_index];
//        current.graph.make_move(best_move);
//        current.update_time(); // �����жϲ���Ϸ���
//        distance_current_to_guiding = all_moves_distance[select_index];
//        if (constexpr double beta = 0.6; all_moves_distance[select_index] < beta * distance_initial_to_guiding)
//        {
//            PathSet.push_back(current.graph);
//            PathSet_makespan.push_back(all_moves_makespan[select_index]);
//        }
//    }
//    // ѡȡ PathSet_makespan ����С����Ϊ���ս��
//    // �ҵ���Сֵ
//    int min_val = *std::ranges::min_element(PathSet_makespan);
//
//    // �ҵ�������Сֵ���±�
//    std::vector<int> min_indices;
//    for (int i = 0; i < PathSet_makespan.size(); ++i)
//    {
//        if (PathSet_makespan[i] == min_val)
//        {
//            min_indices.push_back(i);
//        }
//    }
//    Graph & final = PathSet[min_indices[RAND_INT(min_indices.size())]];
//    return Schedule{ final, current.operation_list };
//}
//int Schedule::similarity(const Schedule& other) const
//{
//    int result = 0;
//    for (auto op = 1; op < this->graph.node_num - 1; ++op)
//    {
//        if (this->graph.on_machine[op] != other.graph.on_machine[op])
//        {
//            result++;
//        }
//        else
//        {
//            if (this->graph.on_machine_pos_vec[op] != other.graph.on_machine_pos_vec[op])
//            {
//                result++;
//            }
//        }
//    }
//    return result;
//}
int change_machine_evaluate(const Schedule& schedule, const NeighborhoodMove& move,
    const std::vector<int>& intersection)
{
    (void)intersection;
    try
    {
        Schedule tmp = schedule;
        tmp.make_move(move);
        return tmp.get_makespan();
    }
    catch (...)
    {
        return INT_MAX / 4;
    }
}

#ifdef EVALUATE_1
int same_machine_evaluate(const Schedule& schedule, const NeighborhoodMove& move)
{
    try
    {
        Schedule tmp = schedule;
        tmp.make_move(move);
        return tmp.get_makespan();
    }
    catch (...)
    {
        return INT_MAX / 4;
    }
}

#else
int same_machine_evaluate(const Schedule& current_schedule, const NeighborhoodMove& action)
{
    try
    {
        Schedule tmp = current_schedule;
        tmp.make_move(action);
        return tmp.get_makespan();
    }
    catch (...)
    {
        return INT_MAX / 4;
    }
}
#endif

