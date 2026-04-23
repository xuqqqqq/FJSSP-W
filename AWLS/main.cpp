//
// Created by qiming on 25-4-12.
//

#include "Instance.h"
#include "RandomGenerator.h"
#include "Solver.h"
#include "DebugTrace.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <string>

int main(int argc, char** argv)
{
#ifdef _WIN32
    // Avoid blocking crash popups during batch runs.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
    awls_trace::log("main: argc=", argc);
    // Usage:
    //   AWLS.exe <time_limit_sec> <rand_seed> [best] [instance_file]
    // If instance_file is provided, load from file; otherwise read from stdin.
    Instance instance;
    if (argc > 4)
    {
        awls_trace::log("main: loading file ", argv[4]);
        load_instance(argv[4], instance);
    }
    else
    {
        awls_trace::log("main: reading instance from stdin");
        std::cin >> instance;
    }
    const long long secTimeout = atoll(argv[1]);
    const int randSeed = atoi(argv[2]);
    int best = 0;
    if (argc > 3)
        best = atoi(argv[3]);
    SET_RAND_SEED(randSeed);
    awls_trace::log("main: timeout=", secTimeout, " seed=", randSeed, " best=", best,
        " jobs=", instance.job_num, " machines=", instance.machine_num,
        " workers=", instance.worker_num, " ops=", instance.op_num,
        " hasWorkers=", instance.has_worker_flexibility);
    auto start = std::chrono::high_resolution_clock::now();

    awls_trace::log("main: entering Solver::Solve");
    Solver::Solve(instance, secTimeout, best);
    awls_trace::log("main: returned from Solver::Solve");

    const auto end = std::chrono::high_resolution_clock::now();

    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::clog << "Function took " << duration.count() << " milliseconds to execute." << std::endl;

    return 0;
}

