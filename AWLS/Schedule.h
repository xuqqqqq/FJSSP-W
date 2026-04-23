#ifndef FJSP_SCHEDULE_H
#define FJSP_SCHEDULE_H

#include <memory>
#include "Graph.h"
#include "NeighborhoodMove.h"

struct OperationTimeInfo
{
    int operator_id;
    int forward_path_length;
    int backward_path_length;
    int end_time;
};

class Schedule
{
    friend class TabuSearch;
public:
    explicit Schedule(const Graph& graph, const std::shared_ptr<OperationList>& operation_list) :
        graph(graph), operation_list(operation_list)
    {
        time_info.resize(graph.node_num);
        assigned_worker.assign(graph.node_num, -1);
        assigned_duration.assign(graph.node_num, 0);
    }
    Schedule() = default;

    Schedule(const Instance& instance, const std::shared_ptr<OperationList>& operation_list) :
        operation_list(operation_list)
    {
        graph.random_init(instance, *operation_list);
        time_info.resize(graph.node_num);
        assigned_worker.assign(graph.node_num, -1);
        assigned_duration.assign(graph.node_num, 0);
        update_time();
    }

    void update_time();

    [[maybe_unused]] void export_schedule(const char* filename) const;

    static constexpr int gamma = 40, beta = 500, theta = 5;

    friend int same_machine_evaluate(const Schedule& schedule, const NeighborhoodMove& move);
    friend int change_machine_evaluate(const Schedule& schedule, const NeighborhoodMove& move,
        const std::vector<int>& intersection);
    friend Schedule path_relinking(const Schedule& initial, const Schedule& guiding);
    friend int delta_distance(const Schedule& current, const Schedule& guiding, const NeighborhoodMove& move);

    [[nodiscard]] int distance(const Schedule& other) const;
    [[nodiscard]] int get_makespan() const { return makespan; }
    [[nodiscard]] bool is_legal_move(const NeighborhoodMove& move) const;
    [[nodiscard]] int similarity(const Schedule& other) const;

    [[nodiscard]] int get_assigned_worker(int op_id) const
    {
        if (op_id < 0 || op_id >= static_cast<int>(assigned_worker.size()))
            return -1;
        return assigned_worker[op_id];
    }

    void output() const;

private:
    int makespan{};
    Graph graph;
    std::shared_ptr<OperationList> operation_list;
    std::vector<OperationTimeInfo> time_info;
    std::vector<int> assigned_worker;
    std::vector<int> assigned_duration;

    [[nodiscard]] bool is_critical_operation(int operation_id) const;

    void make_move(const NeighborhoodMove& move);
};

inline bool Schedule::is_critical_operation(int operation_id) const
{
    return (time_info[operation_id].end_time + time_info[operation_id].backward_path_length == makespan);
}

#endif // FJSP_SCHEDULE_H
