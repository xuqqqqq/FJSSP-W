#include "Solver.h"
#include <chrono>
#include "DebugTrace.h"
#include <utility>

namespace
{
int construction_sample_count(const Instance& instance)
{
    if (!instance.has_worker_flexibility)
    {
        return 1;
    }
    if (instance.job_num <= 2 && instance.machine_num <= 2)
    {
        return 4;
    }
    return 1;
}

Schedule sample_best_construction(const Instance& instance,
    const std::shared_ptr<OperationList>& operation_list,
    const int sample_count)
{
    Schedule best_schedule(instance, operation_list);
    for (int i = 1; i < sample_count; ++i)
    {
        Schedule candidate(instance, operation_list);
        if (candidate.get_makespan() < best_schedule.get_makespan())
        {
            best_schedule = std::move(candidate);
        }
    }
    return best_schedule;
}
}

// ��̬ԭ�ӱ��������ڿ����㷨ִ�е�ֹͣ��־
std::atomic<bool> Solver::stop_flag{false};


void Solver::run_timer_thread(long long time_limit)
{
    // ����ָ��ʱ�䣨ת��Ϊ���룬����ǰ300���봥����
    std::this_thread::sleep_for(std::chrono::milliseconds(time_limit * 1000 - 300));
    // ����ֹͣ��־��ʹ�ÿ����ڴ�����Ϊ����Ҫ�ϸ��ͬ����
    stop_flag.store(true, std::memory_order_relaxed);
}


void Solver::initialize_population(const Instance& instance, std::shared_ptr<OperationList>& operation_list,
    std::vector<Schedule>& population)
{
    // ���������б������������ҵ����
    operation_list = std::make_shared<OperationList>(instance);
    // ������Ⱥ��СΪ1
    population.resize(1);
    population[0] = sample_best_construction(instance, operation_list, construction_sample_count(instance));
}


void Solver::update_best_solution(const std::vector<Schedule>& population, Schedule& best_solution, int best)
{
    // ����Ⱥ��һ��������Ѱ��makespan��С��
    const auto& temp_best =
        std::min_element(population.begin(), population.begin() + 1,
            [](const Schedule& a, const Schedule& b) { return a.get_makespan() < b.get_makespan(); });
    
    // ����ҵ����õĽ⣬�������Ž�
    if (temp_best->get_makespan() < best_solution.get_makespan())
    {
        best_solution = *temp_best;
    }
}


//void Solver::population_maintenance(std::vector<Schedule>& population, const Instance& instance,
//    std::shared_ptr<OperationList>& operation_list, int gen)
//{
//    // ÿ10������һ����Ⱥ����
//    if (gen % 10 == 0)
//    {
//        // �ƶ�����λ�ã������µ��������
//        population[1] = std::move(population[3]);  // ����4�������Ƶ���2��λ��
//        population[3] = std::move(population[2]);  // ����3�������Ƶ���4��λ��
//        population[2] = Schedule(instance, operation_list);  // �ڵ�3��λ�ô����¸���
//    }
//
//    // ���ǰ������������ƶȣ���������������滻����һ��
//    if (population[0].similarity(population[1]) < instance.op_num * 0.1)
//    {
//        // ���ѡ���1�����2����������滻
//        int idx = RAND_INT(2);
//        population[idx] = Schedule(instance, operation_list);
//    }
//}


void Solver::Solve(const Instance& instance, long long time_limit, int best, SolverMode mode)
{
    awls_trace::log("Solver::Solve start time_limit=", time_limit, " best=", best, " mode=", static_cast<int>(mode));
    // ����ֹͣ��־
    stop_flag.store(false, std::memory_order_relaxed);
    
    // �����ʱ���̣߳���ʱ�����Ƶ���ʱ����ֹͣ��־
    std::thread timer_thread(run_timer_thread, time_limit);
    timer_thread.detach();  // �����̣߳������������

    // �㷨���ݽṹ��ʼ��
    std::shared_ptr<OperationList> operation_list;  // �����б��������ҵ�����ļ��ϣ�
    std::vector<Schedule> population;               // ��Ⱥ��������ȷ�����
    initialize_population(instance, operation_list, population);
    awls_trace::log("Solver::Solve initialized population size=", population.size(),
        " initial_makespan=", population.empty() ? -1 : population.front().get_makespan());

    // ��¼���Ž���㷨״̬
    Schedule best_solution = population.front();    // ��ʼ���Ž���Ϊ��һ������
    //std::vector<Schedule> children(2);              // �Ӵ���������
    std::vector<Schedule> schedule(1);              // ������
    int gen = 1;                                    // ����������

    // ��ѭ����ֱ��ʱ��ľ����ҵ����Ž�
    while (!stop_flag.load(std::memory_order_relaxed))
    {
        if (gen <= 5 || gen % 50 == 0)
        {
            awls_trace::log("Solver::Solve generation=", gen,
                " population0=", population.empty() ? -1 : population[0].get_makespan(),
                " best=", best_solution.get_makespan());
        }
#ifdef PRINT_INFO
        auto start = std::chrono::high_resolution_clock::now();  // ��ʼ��ʱ�����ڵ�����Ϣ��
#endif

        // ========== ����1: ·��������̽����ռ䣩 ==========
        // ��������������֮��ִ��˫��·�����������������Ӵ�
        //children[0] = path_relinking(population[0], population[1]);  // �Ӹ���0������1��·��
        //children[1] = path_relinking(population[1], population[0]);  // �Ӹ���1������0��·��
        schedule[0] = population[0];

        // ========== ����2: ����ʱ����㣨���д���� ==========
        // ����ģʽѡ���л��и����Ӵ���ʱ�䰲��
        if (mode == PARALLEL)
        {
#pragma omp parallel for  // OpenMP���л�
            for (int i = 0; i < 1; ++i)
            {
                schedule[i].update_time();
                //children[i].update_time();  // ���¼���ÿ�������Ŀ�ʼ�ͽ���ʱ��
            }
        }
        else  // ����ģʽ
        {
            schedule[0].update_time();
            if (gen <= 5 || gen % 50 == 0)
            {
                awls_trace::log("Solver::Solve after update_time gen=", gen,
                    " schedule0=", schedule[0].get_makespan());
            }
            //children[0].update_time();
            //children[1].update_time();
        }

        // ========== ����3: �����������ֲ��Ż��� ==========
        // ����һ����������ʵ�����Ż�
        std::vector<TabuSearch> TS(1, TabuSearch{ instance });

        if (mode == PARALLEL)
        {
#pragma omp parallel for
            for (int i = 0; i < 1; ++i)
            {
                // ��ÿ���Ӵ�ִ�н�������������ֹͣ��־�Ա���ǰ��ֹ
                TS[i].search(schedule[i], stop_flag);
                //TS[i].search(children[i], stop_flag);
            }
        }
        else
        {
            if (gen <= 5 || gen % 50 == 0)
            {
                awls_trace::log("Solver::Solve before TS gen=", gen,
                    " schedule0=", schedule[0].get_makespan());
            }
            TS[0].search(schedule[0], stop_flag);
            if (gen <= 5 || gen % 50 == 0)
            {
                awls_trace::log("Solver::Solve after TS gen=", gen,
                    " best_schedule0=", TS[0].best_schedule.get_makespan());
            }
            //TS[0].search(children[0], stop_flag);
            //TS[1].search(children[1], stop_flag);
        }

        // ========== ����4: ������Ⱥ ==========
        // �ý��������Ż���Ľ��滻ԭ��Ⱥ�еĸ���
        if (mode == PARALLEL)
        {
#pragma omp parallel for
            for (int i = 0; i < 1; ++i)
            {
                population[i] = TS[i].best_schedule;  // ����Ϊ���������ҵ�����ѵ���
            }
        }
        else
        {
            population[0] = TS[0].best_schedule;
            //population[1] = TS[1].best_schedule;
        }

        // ����ȫ�����Ž�
        update_best_solution(population, best_solution, best);
        if (gen <= 5 || gen % 50 == 0)
        {
            awls_trace::log("Solver::Solve after update_best gen=", gen,
                " population0=", population[0].get_makespan(),
                " best=", best_solution.get_makespan());
        }

        // ����Ƿ��ҵ���֪���Ž⣬���������ǰ��ֹ
        if (best_solution.get_makespan() <= best)
            break;

#ifdef PRINT_INFO
        // �����ǰ������Ϣ�����ڵ��Ժͼ���㷨���ȣ�
        std::clog << "gen: " << gen << "\tS1: " << population[0].get_makespan()
            << "\tS2: " << population[1].get_makespan() << "\tbest: " << best_solution.get_makespan()
            << std::endl;
#endif

        // ========== ����5: ��Ⱥά�� ==========
        // ���������¸��壬�����Ⱥ������
        //population_maintenance(population, instance, operation_list, gen);
        gen++;  // ��������

#ifdef PRINT_INFO
        // ���㲢�������ִ��ʱ��
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::clog << "Gen took " << duration.count() << " milliseconds to execute." << std::endl;
#endif
    }

    // ========== �㷨������������ ==========
    awls_trace::log("Solver::Solve finished best=", best_solution.get_makespan());
    std::clog << "Final solution: " << best_solution.get_makespan() << std::endl;
    best_solution.output();                            // �ڿ���̨���������Ϣ
    best_solution.export_schedule("../../output.csv"); // �������Ƚ����CSV�ļ�
}
