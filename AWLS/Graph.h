//
// Created by XQQ on 25-11-12.
//

#ifndef FJSP_GRAPH_H
#define FJSP_GRAPH_H

#include <deque>
#include <vector>
#include "Instance.h"
#include "NeighborhoodMove.h"
#include "Operation.h"

/**
 * Disjunctive graph representation for flexable job shop scheduling.
 * Total number of nodes = job_num * operation_num + 2 (including virtual start and end nodes)
 */
struct Graph
{
    int node_num;
    /**
     * Forward edges in the disjunctive graph.
     * For all nodes except head virtual nodes:
     * - edges[u] = v indicates an edge u -> v
     * - edges[u] = -1 indicates no outgoing edge from u
     *
     * Virtual nodes' outgoing edges are handled separately via
     * first_job_operation and first_machine_operation.
     */
    std::vector<int> job_successor; // Successor in job sequence
    std::vector<int> machine_successor; // Successor in machine sequence
    std::vector<int> worker_successor; // Successor in worker sequence

    /**
     * Backward edges in the disjunctive graph.
     * For all nodes except tail virtual nodes:
     */
    std::vector<int> job_predecessor; // Predecessor in job sequence
    std::vector<int> machine_predecessor; // Predecessor in machine sequence
    std::vector<int> worker_predecessor; // Predecessor in worker sequence

    /**
     * Special edge handling for virtual nodes' outgoing edges:
     */
    std::vector<int> first_job_operation; // First operation ID for each job
    std::vector<int> last_job_operation; // Last operation ID for each job
    std::vector<int> first_machine_operation; // First operation ID for each machine
    std::vector<int> last_machine_operation; // Last operation ID for each machine
    std::vector<int> first_worker_operation; // First operation ID for each worker
    std::vector<int> last_worker_operation; // Last operation ID for each worker

    std::vector<int> on_machine; // Assigned machine for each operation
    std::vector<int> on_worker; // Assigned worker for each operation
    std::vector<int> machine_operation_count; // Number of operations assigned to each machine
    std::vector<int> worker_operation_count; // Number of operations assigned to each worker
    std::vector<int> on_machine_pos_vec; // Position inside the machine chain
    std::vector<int> on_worker_pos_vec; // Position inside the worker chain

    /**
     * Performs topological sort on the graph
     * @param reverse If true, performs reverse topological sort
     * @return Deque containing node IDs in topologically sorted order
     */
    [[nodiscard]] std::deque<int> topological_sort(bool reverse = false, bool include_worker = true) const;

    /**
     * Generates a feasible initial solution graph from problem instance.
     * The current implementation uses a deterministic worker-aware dispatching
     * rule; the historical name is kept to avoid touching call sites.
     */
    void random_init(const Instance& instance, const OperationList& operation_list, int construction_variant = 0);

    void make_move(const NeighborhoodMove& move);


    [[maybe_unused]] [[deprecated("Please use on_machine_pos_vec[op_id] instead.")]] [[nodiscard]] int
        on_machine_pos(int op_id) const;

    [[nodiscard]] int distance(const Graph& other) const;
};

#endif // FJSP_GRAPH_H

