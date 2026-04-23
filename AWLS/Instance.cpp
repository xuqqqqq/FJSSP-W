#include "Instance.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::vector<int> parse_int_line(const std::string& line)
{
    std::istringstream iss(line);
    std::vector<int> values;
    int value = 0;
    while (iss >> value)
    {
        values.push_back(value);
    }
    return values;
}

bool parse_job_line_fjssp(const std::string& line, std::vector<std::vector<Candidate>>& job)
{
    const auto tokens = parse_int_line(line);
    if (tokens.empty())
    {
        return false;
    }

    int index = 0;
    const int operation_count = tokens[index++];
    if (operation_count < 0)
    {
        return false;
    }

    job.clear();
    job.resize(operation_count);

    for (int op = 0; op < operation_count; ++op)
    {
        if (index >= static_cast<int>(tokens.size()))
        {
            return false;
        }
        const int candidate_count = tokens[index++];
        if (candidate_count <= 0)
        {
            return false;
        }

        auto& operation = job[op];
        operation.reserve(candidate_count);

        for (int c = 0; c < candidate_count; ++c)
        {
            if (index + 1 >= static_cast<int>(tokens.size()))
            {
                return false;
            }
            Candidate candidate;
            candidate.machine = tokens[index++];
            candidate.duration = tokens[index++];
            candidate.worker_options.push_back({0, candidate.duration});
            operation.push_back(std::move(candidate));
        }
    }

    return index == static_cast<int>(tokens.size());
}

bool parse_job_line_fjsspw(const std::string& line, std::vector<std::vector<Candidate>>& job)
{
    const auto tokens = parse_int_line(line);
    if (tokens.empty())
    {
        return false;
    }

    int index = 0;
    const int operation_count = tokens[index++];
    if (operation_count < 0)
    {
        return false;
    }

    job.clear();
    job.resize(operation_count);

    for (int op = 0; op < operation_count; ++op)
    {
        if (index >= static_cast<int>(tokens.size()))
        {
            return false;
        }

        const int machine_count = tokens[index++];
        if (machine_count <= 0)
        {
            return false;
        }

        auto& operation = job[op];
        operation.reserve(machine_count);

        for (int m = 0; m < machine_count; ++m)
        {
            if (index + 1 >= static_cast<int>(tokens.size()))
            {
                return false;
            }

            Candidate candidate;
            candidate.machine = tokens[index++];
            const int worker_count = tokens[index++];
            if (worker_count <= 0)
            {
                return false;
            }

            candidate.duration = std::numeric_limits<int>::max();
            candidate.worker_options.reserve(worker_count);

            for (int w = 0; w < worker_count; ++w)
            {
                if (index + 1 >= static_cast<int>(tokens.size()))
                {
                    return false;
                }

                const int worker = tokens[index++];
                const int duration = tokens[index++];
                candidate.worker_options.push_back({worker, duration});
                candidate.duration = std::min(candidate.duration, duration);
            }

            if (candidate.duration == std::numeric_limits<int>::max())
            {
                return false;
            }

            operation.push_back(std::move(candidate));
        }
    }

    return index == static_cast<int>(tokens.size());
}

void normalize_machine_indices(Instance& instance)
{
    bool has_zero = false;
    for (const auto& job : instance.jobs)
    {
        for (const auto& operation : job)
        {
            for (const auto& candidate : operation)
            {
                if (candidate.machine == 0)
                {
                    has_zero = true;
                    break;
                }
            }
            if (has_zero)
            {
                break;
            }
        }
        if (has_zero)
        {
            break;
        }
    }

    const bool one_based = !has_zero;
    for (auto& job : instance.jobs)
    {
        for (auto& operation : job)
        {
            for (auto& candidate : operation)
            {
                if (one_based)
                {
                    candidate.machine -= 1;
                }
                if (candidate.machine < 0 || candidate.machine >= instance.machine_num)
                {
                    throw std::runtime_error("Machine id out of range while parsing instance.");
                }
            }
        }
    }
}

void normalize_worker_indices(Instance& instance)
{
    if (!instance.has_worker_flexibility)
    {
        instance.worker_num = 1;
        return;
    }

    bool has_zero = false;
    int max_worker = -1;

    for (const auto& job : instance.jobs)
    {
        for (const auto& operation : job)
        {
            for (const auto& candidate : operation)
            {
                for (const auto& worker_option : candidate.worker_options)
                {
                    if (worker_option.worker == 0)
                    {
                        has_zero = true;
                    }
                    max_worker = std::max(max_worker, worker_option.worker);
                }
            }
        }
    }

    const bool one_based = !has_zero;
    max_worker = -1;

    for (auto& job : instance.jobs)
    {
        for (auto& operation : job)
        {
            for (auto& candidate : operation)
            {
                for (auto& worker_option : candidate.worker_options)
                {
                    if (one_based)
                    {
                        worker_option.worker -= 1;
                    }
                    if (worker_option.worker < 0)
                    {
                        throw std::runtime_error("Worker id out of range while parsing instance.");
                    }
                    max_worker = std::max(max_worker, worker_option.worker);
                }
            }
        }
    }

    instance.worker_num = std::max(instance.worker_num, max_worker + 1);
}

void finalize_counts(Instance& instance)
{
    instance.op_num = 0;
    instance.max_candidate_num = 0;

    for (const auto& job : instance.jobs)
    {
        instance.op_num += static_cast<int>(job.size());
        for (const auto& operation : job)
        {
            instance.max_candidate_num = std::max(instance.max_candidate_num, static_cast<int>(operation.size()));
        }
    }
}

} // namespace

std::ostream& operator<<(std::ostream& os, const Instance& instance)
{
    if (instance.has_worker_flexibility)
    {
        os << instance.job_num << " " << instance.machine_num << " " << instance.worker_num << std::endl;
    }
    else
    {
        os << instance.job_num << " " << instance.machine_num << " " << instance.max_candidate_num << std::endl;
    }

    for (const auto& job : instance.jobs)
    {
        os << job.size() << " ";
        for (const auto& operation : job)
        {
            os << operation.size() << " ";
            for (const auto& candidate : operation)
            {
                if (instance.has_worker_flexibility)
                {
                    os << candidate.machine << " " << candidate.worker_options.size() << " ";
                    for (const auto& worker_option : candidate.worker_options)
                    {
                        os << worker_option.worker << " " << worker_option.duration << " ";
                    }
                }
                else
                {
                    os << candidate.machine << " " << candidate.duration << " ";
                }
            }
        }
        os << std::endl;
    }
    return os;
}

std::istream& operator>>(std::istream& is, Instance& instance)
{
    instance = Instance{};

    std::string header_line;
    while (std::getline(is, header_line))
    {
        if (!header_line.empty() && parse_int_line(header_line).size() > 0)
        {
            break;
        }
    }

    if (header_line.empty())
    {
        return is;
    }

    const auto header_tokens = parse_int_line(header_line);
    if (header_tokens.size() < 2)
    {
        is.setstate(std::ios::failbit);
        return is;
    }

    instance.job_num = header_tokens[0];
    instance.machine_num = header_tokens[1];
    const int third_header_value = header_tokens.size() >= 3 ? header_tokens[2] : 0;

    std::vector<std::string> job_lines;
    job_lines.reserve(instance.job_num);
    std::string line;
    while (job_lines.size() < static_cast<size_t>(instance.job_num) && std::getline(is, line))
    {
        if (parse_int_line(line).empty())
        {
            continue;
        }
        job_lines.push_back(line);
    }

    if (job_lines.size() != static_cast<size_t>(instance.job_num))
    {
        is.setstate(std::ios::failbit);
        return is;
    }

    bool parsed_as_worker = false;
    if (third_header_value > 0)
    {
        std::vector<std::vector<std::vector<Candidate>>> jobs;
        jobs.resize(instance.job_num);

        bool ok = true;
        for (int job_id = 0; job_id < instance.job_num; ++job_id)
        {
            std::vector<std::vector<Candidate>> parsed_job;
            if (!parse_job_line_fjsspw(job_lines[job_id], parsed_job))
            {
                ok = false;
                break;
            }
            jobs[job_id] = std::move(parsed_job);
        }

        if (ok)
        {
            instance.jobs = std::move(jobs);
            instance.worker_num = third_header_value;
            instance.has_worker_flexibility = true;
            normalize_machine_indices(instance);
            normalize_worker_indices(instance);
            finalize_counts(instance);
            parsed_as_worker = true;
        }
    }

    if (!parsed_as_worker)
    {
        std::vector<std::vector<std::vector<Candidate>>> jobs;
        jobs.resize(instance.job_num);

        for (int job_id = 0; job_id < instance.job_num; ++job_id)
        {
            std::vector<std::vector<Candidate>> parsed_job;
            if (!parse_job_line_fjssp(job_lines[job_id], parsed_job))
            {
                is.setstate(std::ios::failbit);
                return is;
            }
            jobs[job_id] = std::move(parsed_job);
        }

        instance.jobs = std::move(jobs);
        instance.has_worker_flexibility = false;
        instance.worker_num = 1;
        normalize_machine_indices(instance);
        normalize_worker_indices(instance);
        finalize_counts(instance);
    }

    return is;
}

void load_instance(const char* file_name, Instance& instance)
{
    std::ifstream file(file_name);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open instance: " + std::string(file_name));
    }
    file >> instance;
    file.close();
}

Instance::Instance(const char* filename)
{
    load_instance(filename, *this);
}
