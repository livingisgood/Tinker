#pragma once
#include <cstdint>
#include <vector>

#include "TinkerAssert.h"

namespace TK
{
    template<typename CostType>
    constexpr CostType GetInfiCost()
    {
        static_assert(sizeof(CostType) != sizeof(CostType), "You need to specify infinity value for this type!");
        return CostType {};
    }

    template<>
    constexpr int GetInfiCost()
    {
        return 0x3f3f3f3f;
    }

    template<>
    constexpr std::int64_t GetInfiCost()
    {
        return 0x3f3f3f3f3f3f3f3f;
    }

    // given n jobs (n stores in JobsNum) and m workers (m stores in WorkersNum) (n <= m)
    // i-th job's cost for j-th worker is given by Costs[i * WorkersNum + j]
    // this solver finds an assignment for each job such that the total cost is minimal.
    // returns output such that output[i] gives the worker for job i.

    // Complexity O(n^2 * m)
    template<typename CostType>
    class THungarianSolver
    {
    public:

        std::vector<CostType> Costs;

        int JobsNum {};
        int WorkersNum {};

        // https://cp-algorithms.com/graph/hungarian-algorithm.html
        const std::vector<int>& Solve(CostType Infi = GetInfiCost<CostType>())
        {
            TK_ASSERT(JobsNum > 0);
            TK_ASSERT(WorkersNum > 0);
            TK_ASSERT(JobsNum <= WorkersNum);
            TK_ASSERT(JobsNum * WorkersNum <= static_cast<int>(Costs.size()));
            
            const CostType Zero = CostType(0);

            // add a dummy job and a dummy worker for convenience;
            // job 0 and worker 0 are dummies.

            // weight for each job.
            WeightJ.clear();
            WeightJ.resize(JobsNum + 1, Zero);

            // weight for each worker.
            WeightW.clear();
            WeightW.resize(WorkersNum + 1, Zero);

            // job for each worker.
            MatchW.clear();
            MatchW.resize(WorkersNum + 1, Zero);

            // prev job in search path.
            Prev.clear();
            Prev.resize(WorkersNum + 1, Zero);

            for (int i = 1; i <= JobsNum; ++i)
            {
                // this loop is to find an augmenting path starts with job i.

                // Let Z1 be the set of reachable Jobs
                // Let Z2 be the set of reachable Workers

                // Z1 and Z2 are currently empty.

                // InPathW[j] != 0 if Worker j is in Z2.
                InPathW.clear();
                InPathW.resize(WorkersNum + 1, 0);

                // MinDelta stores the min delta between every jobs in Z1 to each worker.
                MinDelta.clear();
                MinDelta.resize(WorkersNum + 1, Infi);

                // Now we have a new worker ready to be added to Z2
                int NewWorker = 0;

                // make dummy worker 0 match to this new job i, this will make us an invariant during loop:
                // every job in Z1 is matched a worker in Z2.
                // to find every job in Z1 we could do so by trace every worker in Z2.
                MatchW[NewWorker] = i;

                do
                {
                    // a new worker is added to Z2,
                    InPathW[NewWorker] = 1;

                    // new worker's matching job is added to Z1
                    int NewJob = MatchW[NewWorker];

                    // now we need to expand Z2, to do so, we need to find the minimal delta edge
                    // between Z1 and Workers not in Z2 and its corresponding Worker. this worker
                    // will be added to Z2. since Z1 is expanded (New Job is added to it), we should
                    // update MinDelta and find the minimal MinDelta.
                    CostType Delta = Infi;
                    int NextWorker = 0;

                    for (int Worker = 1; Worker <= WorkersNum; ++Worker)
                    {
                        // for every worker j that is not in Z2
                        if (InPathW[Worker] == Zero)
                        {
                            CostType CurDelta = GetCost(NewJob, Worker) - WeightJ[NewJob] - WeightW[Worker];

                            if (CurDelta < MinDelta[Worker])
                            {
                                MinDelta[Worker] = CurDelta;
                                Prev[Worker] = NewWorker;
                            }

                            if (MinDelta[Worker] < Delta)
                            {
                                Delta = MinDelta[Worker];
                                NextWorker = Worker;
                            }
                        }
                    }

                    for (int Worker = 0; Worker <= WorkersNum; ++Worker)
                    {
                        if (InPathW[Worker] != 0)
                        {
                            WeightW[Worker] -= Delta;
                            WeightJ[MatchW[Worker]] += Delta;
                        }
                        else
                        {
                            MinDelta[Worker] -= Delta;
                        }
                    }

                    NewWorker = NextWorker;
                }
                while (MatchW[NewWorker] != Zero);

                // when the loop above ends, we have an augmenting path now.
                int LastWorker = NewWorker;
                do
                {
                    int NextWorker = Prev[LastWorker];
                    MatchW[LastWorker] = MatchW[NextWorker];
                    LastWorker = NextWorker;
                }
                while (LastWorker != 0);
            }

            Output.clear();
            Output.resize(JobsNum, 0);
            for (int Worker = 1; Worker <= WorkersNum; ++Worker)
            {
                int Job = MatchW[Worker];

                if (Job > 0)
                    Output[Job - 1] = Worker - 1;
            }

            return Output;
        }

    private:

        CostType GetCost(int Job, int Worker) const
        {
            return Costs[(Job - 1) * WorkersNum + Worker - 1];
        }
        
        std::vector<int> Output;

        std::vector<CostType> WeightJ;
        std::vector<CostType> WeightW;
        std::vector<int> MatchW;
        std::vector<int> Prev;
        std::vector<CostType> MinDelta;
        std::vector<std::int8_t> InPathW;
    };
}
