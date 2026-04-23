//
// Created by qiming on 25-4-13.
//

#include "TabuSearch.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <set>
#include "RandomGenerator.h"
#include "Schedule.h"

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
    current_schedule = schedule;
    best_schedule = schedule;
    Schedule prev_schedule;
    iteration = 0;
    while (iteration < 10000)
    {
        auto move = find_move();
        if (move.which == 0)
        {
            continue;
        }
        prev_schedule = current_schedule;
        make_move(move);
         // ��ȡ�ؼ�·������
        std::vector<int> critical_ops;
        for (int i = 1; i < current_schedule.graph.node_num - 1; ++i) {
            if (current_schedule.is_critical_operation(i)) {
                critical_ops.push_back(i);
            }
        }
        
        // ���±��ƶ�������Ȩ�غͿ��м���
        UpdateWeight_per_op(current_schedule.operation_list->operations, 
                           move.which, 
                           critical_ops,
                           best_schedule.makespan,  // fStar - ��ʷ����makespan
                           prev_schedule.makespan,  // fPrev - ǰһ�����makespan  
                           current_schedule.makespan, // fCurr - ��ǰ���makespan
                           Schedule::beta, 
                           Schedule::gamma, 
                           Schedule::theta);
        
        

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


NeighborhoodMove TabuSearch::find_move()
{
    std::vector<NeighborhoodMove> all_moves;
    std::vector<NeighborhoodMove> best_moves;
    bool find_all = false;
    int min_makespan = INT_MAX;
    do
    {
        if (!find_all)
            update_critical_block();
        else
            update_all_critical_block();
        for (const auto& block : critical_blocks)
        {
            if (block.empty())
            {
                continue;
            }
            for (int i = 0; i < block.size(); i++)
            {
                const int op = block[i];
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
                    cur_op != -1 && cur_op != block.front();
                    cur_op = current_schedule.graph.machine_successor[cur_op])
                {
                    NeighborhoodMove move{ Method::FRONT, op, cur_op };
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }


                for (int cur_op = current_schedule.graph.machine_successor[block.back()]; cur_op != -1; cur_op = current_schedule.graph.machine_successor[cur_op])
                {
                    NeighborhoodMove move{ Method::BACK, op, cur_op };
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }
            }

            if (const int n = static_cast<int>(block.size()); n == 2)
            {
                // ����ؼ���Ĵ�СΪ2����������ֻ��һ��
                if (block[0] > 0 && block[0] < current_schedule.graph.node_num - 1 &&
                    block[1] > 0 && block[1] < current_schedule.graph.node_num - 1)
                {
                    NeighborhoodMove move{ Method::BACK, block[0], block[1] };
                // LEGAL_PUSH(all_moves, move)
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }
            }
            else
            {
                // ������Ԫ�ز������Ԫ��֮��
                for (int j = 2; j < n; j++)
                {
                    if (block.front() <= 0 || block.front() >= current_schedule.graph.node_num - 1 ||
                        block[j] <= 0 || block[j] >= current_schedule.graph.node_num - 1)
                    {
                        continue;
                    }
                    NeighborhoodMove move(Method::BACK, block.front(), block[j]);
                    // LEGAL_PUSH(all_moves, move)
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }
                // ����βԪ�ز������Ԫ��֮ǰ
                for (int j = n - 2; j >= 0; j--)
                {
                    if (block.back() <= 0 || block.back() >= current_schedule.graph.node_num - 1 ||
                        block[j] <= 0 || block[j] >= current_schedule.graph.node_num - 1)
                    {
                        continue;
                    }
                    NeighborhoodMove move(Method::FRONT, block.back(), block[j]);
                    // LEGAL_PUSH(all_moves, move)
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }
                // ������Ԫ�ز������Ԫ��֮ǰ
                for (int j = 1; j < n - 1; j++)
                {
                    if (block[j] <= 0 || block[j] >= current_schedule.graph.node_num - 1 ||
                        block.front() <= 0 || block.front() >= current_schedule.graph.node_num - 1)
                    {
                        continue;
                    }
                    NeighborhoodMove move(Method::FRONT, block[j], block.front());
                    // LEGAL_PUSH(all_moves, move)
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }
                // ������Ԫ�ز����βԪ��֮��
                for (int j = 1; j < n - 1; j++)
                {
                    if (block[j] <= 0 || block[j] >= current_schedule.graph.node_num - 1 ||
                        block.back() <= 0 || block.back() >= current_schedule.graph.node_num - 1)
                    {
                        continue;
                    }
                    NeighborhoodMove move(Method::BACK, block[j], block.back());
                    // LEGAL_PUSH(all_moves, move)
                    same_machine_evaluate_and_push(current_schedule, move, all_moves, best_moves, min_makespan);
                }
            }
        }


        // Temporary safety switch:
        // disable change-machine neighborhood while stabilizing FJSSP-W integration.
        constexpr bool kEnableChangeMachineNeighborhood = false;
        if (!find_all && kEnableChangeMachineNeighborhood)
        {
            for (auto op = 1; op < current_schedule.graph.node_num - 1; op++)
            {
                if (current_schedule.is_critical_operation(op))
                {
                    const int old_machine = current_schedule.graph.on_machine[op];
                    if (current_schedule.graph.machine_operation_count[old_machine] <= 1)
                    {
                        continue;
                    }
                    for (const auto candidate_machine : (*current_schedule.operation_list)[op].candidates)
                    {
                        if (candidate_machine != old_machine)
                        {
                            std::vector<int> RK;
                            std::vector<int> LK;
                            const int job_pre = current_schedule.graph.job_predecessor[op];
                            const int job_suc = current_schedule.graph.job_successor[op];

                            const int remove_machine_R_op = current_schedule.time_info[job_pre].end_time;
                            int remove_machine_Q_op = 0;
                            if (job_suc != current_schedule.graph.node_num - 1)
                            {
                                remove_machine_Q_op = current_schedule.time_info[job_suc].backward_path_length +
                                    current_schedule.operation_list->duration(
                                        job_suc, current_schedule.graph.on_machine[job_suc]);
                            }
                            const int first_machine_op =
                                current_schedule.graph.first_machine_operation[candidate_machine];
                            for (auto u = first_machine_op; u != -1; u = current_schedule.graph.machine_successor[u])
                            {
                                int t1 = current_schedule.time_info[u].end_time;
                                int t2 = current_schedule.time_info[u].backward_path_length +
                                    current_schedule.operation_list->duration(u, candidate_machine);
                                if (t1 > remove_machine_R_op)
                                {
                                    RK.push_back(u);
                                }

                                if (t2 > remove_machine_Q_op)
                                {
                                    LK.push_back(u);
                                }
                            }

                            // ---- ��LK��RK�Ľ��� ----
                            std::vector<int> intersection;
                            intersection.reserve(std::min(RK.size(), LK.size()));
                            // ��LK��RK�Ľ���
                            if (!LK.empty() && !RK.empty())
                            {
                                // ��� LK �ĳ��Ƚϳ������� LK ��ĩβԪ��ƥ�� RK �Ŀ�ͷԪ��
                                if (LK.size() > RK.size())
                                {
                                    int index = 0;
                                    for (; index < RK.size(); index++)
                                    {
                                        if (RK[index] == LK.back())
                                            break;
                                    }
                                    // ���δ�ҵ�����˵������Ϊ�ռ�
                                    // ���򣬽�������ӵ������
                                    if (index < RK.size())
                                    {
                                        for (int i = 0; i <= index; i++)
                                        {
                                            if (RK[i] == LK[LK.size() - 1 - index + i])
                                            {
                                                intersection.push_back(RK[i]);
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    // ��� RK �ĳ��Ƚϳ������� RK �Ŀ�ͷԪ��ƥ�� LK ��ĩβԪ��
                                    int index = static_cast<int>(LK.size() - 1);
                                    for (; index >= 0; index--)
                                    {
                                        if (LK[index] == RK.front())
                                            break;
                                    }
                                    if (index >= 0)
                                    {
                                        for (int i = index; i < LK.size(); i++)
                                        {
                                            if (LK[i] == RK[i - index])
                                                intersection.push_back(LK[i]);
                                            else
                                                break;
                                        }
                                    }
                                }
                            }


                            if (!intersection.empty())
                            {
                                // all_moves.emplace_back(Method::CHANGE_MACHINE_FRONT, op, intersection.front());
                                change_machine_evaluate_and_push(
                                    current_schedule, { Method::CHANGE_MACHINE_FRONT, op, intersection.front() },
                                    intersection, all_moves, best_moves, min_makespan);
                                for (auto& u : intersection)
                                {
                                    // all_moves.emplace_back(Method::CHANGE_MACHINE_BACK, op, u);
                                    change_machine_evaluate_and_push(current_schedule,
                                        { Method::CHANGE_MACHINE_BACK, op, u }, intersection,
                                        all_moves, best_moves, min_makespan);
                                }
                            }
                            else
                            {
                                if (!LK.empty() && !RK.empty())
                                {
                                    for (int start = LK.back(); start != RK.front();
                                        start = current_schedule.graph.machine_successor[start])
                                    {
                                        // all_moves.emplace_back(Method::CHANGE_MACHINE_BACK, op, start);
                                        change_machine_evaluate_and_push(
                                            current_schedule, { Method::CHANGE_MACHINE_BACK, op, start }, intersection,
                                            all_moves, best_moves, min_makespan);
                                    }
                                }
                                else if (LK.empty() && !RK.empty())
                                {
                                    // all_moves.emplace_back(Method::CHANGE_MACHINE_FRONT, op, RK.front());
                                    change_machine_evaluate_and_push(current_schedule,
                                        { Method::CHANGE_MACHINE_FRONT, op, RK.front() },
                                        intersection, all_moves, best_moves, min_makespan);
                                }
                                else if (!LK.empty() && RK.empty())
                                {
                                    // all_moves.emplace_back(Method::CHANGE_MACHINE_BACK, op, LK.back());
                                    change_machine_evaluate_and_push(current_schedule,
                                        { Method::CHANGE_MACHINE_BACK, op, LK.back() },
                                        intersection, all_moves, best_moves, min_makespan);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (find_all && all_moves.empty())
        {
            // û�кϷ���������
            // ����
            std::clog << "Final solution: " << current_schedule.get_makespan() << std::endl;
            current_schedule.output();
            exit(0);
        }
        find_all = true;
    } while (all_moves.empty());


    if (best_moves.empty())
    {
        // �������һ������
        return all_moves[RAND_INT(static_cast<int>(all_moves.size()))];
    }
    if (min_makespan > current_schedule.makespan && RAND_INT(100) < 3)
    {
        return all_moves[RAND_INT(static_cast<int>(all_moves.size()))];
    }
    // ���������������������һ������
    const int index = RAND_INT(static_cast<int>(best_moves.size()));
    return best_moves[index];
}
#define NEXT_MACHINE_OP(op) current_schedule.graph.machine_successor[op]

void TabuSearch::make_move(const NeighborhoodMove& move)
{
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
}

bool TabuSearch::is_tabu(const NeighborhoodMove& move, const int makespan) const
{
    // ����׼��
    if (makespan < best_schedule.makespan)
    {
        return false;
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

    std::vector<int> start_candidates;
    // ��ÿ�������ĵ�һ������ʼѰ�ң��ҵ��ؼ�·���Ŀ�ͷ
    for (auto start : graph.first_machine_operation)
    {
        if (start != -1 &&
            current_schedule.is_critical_operation(start) &&
            current_schedule.time_info[start].forward_path_length == 0)
        {
            start_candidates.push_back(start);
        }
    }

    if (!start_candidates.empty())
    {
        do
        {
            critical_path.clear();
            // �����һ��start��ʼѰ��
            int start_index = RAND_INT(static_cast<int>(start_candidates.size()));
            critical_path.push_back(start_candidates[start_index]);
            start_candidates[start_index] = start_candidates.back();
            start_candidates.pop_back();
            while (current_schedule.time_info[critical_path.back()].end_time != current_schedule.makespan)
            {
                const auto op = critical_path.back();
                const auto ms_op = graph.machine_successor[op];
                // ̰����ͬ������Ѱ��
                if (ms_op != -1 && current_schedule.is_critical_operation(ms_op) &&
                    current_schedule.time_info[ms_op].forward_path_length == current_schedule.time_info[op].end_time)
                {
                    critical_path.push_back(ms_op);
                    continue;
                }
                const auto js_op = graph.job_successor[op];
                if (js_op == -1 || js_op >= current_schedule.graph.node_num)
                {
                    break;
                }
                critical_path.push_back(js_op);
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
            // ��ʼѰ�ҹؼ���
            critical_blocks.clear();
            auto prev_machine = -1;
            std::vector<int> critical_block;
            for (auto op : critical_path)
            {
                if (graph.on_machine[op] != prev_machine)
                {
                    if (critical_block.size() > 1)
                    {
                        critical_blocks.push_back(critical_block);
                        critical_block.clear();
                    }
                    else
                    {
                        critical_block.clear();
                    }
                    critical_block.push_back(op);
                    prev_machine = graph.on_machine[op];
                }
                else
                {
                    critical_block.push_back(op);
                }
            }
            if (critical_block.size() > 1)
            {
                critical_blocks.push_back(critical_block);
            }
        } while (critical_blocks.empty() && !start_candidates.empty());
    }
    if (critical_blocks.empty())
    {
        // �������л������ҵ������ϵ������Ĺؼ�����
        for (const auto start_machine_op : graph.first_machine_operation)
        {
            if (start_machine_op == -1)
            {
                continue;
            }
            int curr_machine_op = start_machine_op;
            std::vector<int> critical_block;
            while (curr_machine_op != -1)
            {
                // �����ǰ����critical block
                if (current_schedule.is_critical_operation(curr_machine_op))
                {
                    // ����ؼ���Ϊ�գ���ֱ�Ӽ���
                    if (critical_block.empty())
                    {
                        critical_block.push_back(curr_machine_op);
                    }
                    else
                    {
                        int prev_critical_op = critical_block.back();
                        // ���ǰһ���ؼ���Ľ���ʱ����ڵ�ǰ�ؼ���Ŀ�ʼʱ�䣬�����
                        if (current_schedule.time_info[prev_critical_op].end_time ==
                            current_schedule.time_info[curr_machine_op].forward_path_length)
                        {
                            critical_block.push_back(curr_machine_op);
                        }
                        else
                        {
                            if (critical_block.size() > 1)
                            {
                                critical_blocks.push_back(critical_block);
                            }
                            critical_block.clear();
                            critical_block.push_back(curr_machine_op);
                        }
                    }
                }
                else
                {
                    if (critical_block.size() > 1)
                    {
                        critical_blocks.push_back(critical_block);
                        critical_block.clear();
                    }
                }
                curr_machine_op = graph.machine_successor[curr_machine_op];
            }
            if (critical_block.size() > 1)
            {
                critical_blocks.push_back(critical_block);
            }
        }
    }
#ifdef UNIT_TEST
    for (const auto& block : critical_blocks)
    {
        for (int i = 0; i < block.size() - 1; i++)
        {
            assert(current_schedule.time_info[block[i]].end_time ==
                current_schedule.time_info[block[i + 1]].forward_path_length);
            assert(current_schedule.graph.on_machine[block[i]] == current_schedule.graph.on_machine[block[i + 1]]);
        }
    }
#endif
}


void TabuSearch::update_all_critical_block()
{
    // Ѱ�����йؼ�·��
    std::vector<std::vector<int>> all_paths;
    const auto& graph = current_schedule.graph;
    all_paths.reserve(graph.first_job_operation.size() * 2);
    for (const auto op : current_schedule.graph.first_machine_operation)
    {
        if (op != -1 &&
            current_schedule.is_critical_operation(op) &&
            current_schedule.time_info[op].forward_path_length == 0)
        {
            all_paths.emplace_back(1, op);
        }
    }

    for (int i = 0; i < all_paths.size(); ++i)
    {
        auto& current_path = all_paths[i];
        int op = current_path.back();
        while (current_schedule.time_info[op].end_time < current_schedule.makespan)
        {
            const int op_machine_suc = current_schedule.graph.machine_successor[op];
            const int op_job_suc = current_schedule.graph.job_successor[op];
            const bool is_ms_critical = (op_machine_suc != -1) && current_schedule.is_critical_operation(op_machine_suc);
            const bool is_js_critical =
                (op_job_suc != -1 && op_machine_suc != op_job_suc)
                    ? current_schedule.is_critical_operation(op_job_suc)
                    : false;
            if (is_ms_critical && is_js_critical &&
                current_schedule.time_info[op].end_time == current_schedule.time_info[op_job_suc].forward_path_length)
            {
                all_paths.push_back(current_path);
                current_path.push_back(op_machine_suc);
                all_paths.back().push_back(op_job_suc);
            }
            else if (is_ms_critical)
            {
                current_path.push_back(op_machine_suc);
            }
            else if (is_js_critical &&
                current_schedule.time_info[op].end_time ==
                current_schedule.time_info[op_job_suc].forward_path_length)
            {
                current_path.push_back(op_job_suc);
            }
            else
            {
                break;
            }
            op = current_path.back();
        }
    }

    // Ѱ�����йؼ���
    critical_blocks.clear();
    std::set<std::vector<int>> unique_blocks; // ���ڼ���ظ��ļ���

    for (const auto& path : all_paths)
    {
        auto prev_machine = -1;
        std::vector<int> critical_block;
        for (auto op : path)
        {
            if (graph.on_machine[op] != prev_machine)
            {
                if (critical_block.size() > 1)
                {
                    // ����Ƿ��Ѿ�������ͬ�Ŀ�
                    if (!unique_blocks.contains(critical_block))
                    {
                        critical_blocks.push_back(critical_block);
                        unique_blocks.insert(critical_block);
                    }
                }
                critical_block.clear();
                critical_block.push_back(op);
                prev_machine = graph.on_machine[op];
            }
            else
            {
                critical_block.push_back(op);
            }
        }
        if (critical_block.size() > 1)
        {
            // ����Ƿ��Ѿ�������ͬ�Ŀ�
            if (!unique_blocks.contains(critical_block))
            {
                critical_blocks.push_back(critical_block);
                unique_blocks.insert(critical_block);
            }
        }
    }
}

