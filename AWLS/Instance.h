#ifndef FJSP_INSTANCE_H
#define FJSP_INSTANCE_H

#include <iomanip>
#include <vector>

struct WorkerOption {
    int worker{};
    int duration{};
};

struct Candidate {
    int machine{};
    int duration{};
    std::vector<WorkerOption> worker_options;
};

struct Instance {
    int job_num{};
    int machine_num{};
    int max_candidate_num{};
    int worker_num{1};
    bool has_worker_flexibility{false};
    std::vector<std::vector<std::vector<Candidate>>> jobs;
    int op_num{};

    Instance() = default;

    explicit Instance(const char* filename);
};

void load_instance(const char* filename, Instance& instance);

std::istream& operator>>(std::istream& is, Instance& instance);

std::ostream& operator<<(std::ostream& os, const Instance& instance);

#endif // FJSP_INSTANCE_H
