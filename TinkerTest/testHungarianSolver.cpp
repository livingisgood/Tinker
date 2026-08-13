#include "gtest/gtest.h"
#include "Algorithm/HungarianSolver.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <limits>
#include <set>
#include <cstdlib>
#include <string>

// ============================================================================
// Convenience aliases
// ============================================================================

using IntSolver = TK::THungarianSolver<int>;
using Int64Solver = TK::THungarianSolver<std::int64_t>;

// ============================================================================
// Test helpers
// ============================================================================

// Total cost of a given assignment: Assign[i] is the worker chosen for job i.
template <typename CostType>
CostType TotalCostOf(const std::vector<CostType>& Costs,
                     const std::vector<int>& Assign,
                     int WorkersNum)
{
    CostType Total = CostType(0);
    for (int i = 0; i < static_cast<int>(Assign.size()); ++i)
        Total += Costs[i * WorkersNum + Assign[i]];
    return Total;
}

// Brute-force minimum total cost by enumerating every permutation of workers
// and taking the first JobsNum entries as the assignment. Correct (but slow)
// reference for small inputs.
template <typename CostType>
CostType BruteForceMinCost(const std::vector<CostType>& Costs,
                           int JobsNum,
                           int WorkersNum)
{
    std::vector<int> Perm(WorkersNum);
    std::iota(Perm.begin(), Perm.end(), 0);

    CostType Best = std::numeric_limits<CostType>::max();
    do
    {
        CostType Total = CostType(0);
        for (int i = 0; i < JobsNum; ++i)
            Total += Costs[i * WorkersNum + Perm[i]];
        if (Total < Best)
            Best = Total;
    } while (std::next_permutation(Perm.begin(), Perm.end()));

    return Best;
}

// Verify an assignment is a valid matching: size matches, every index is in
// range and every job gets a distinct worker.
void ValidateAssignment(const std::vector<int>& Assign,
                        int JobsNum,
                        int WorkersNum)
{
    ASSERT_EQ(static_cast<int>(Assign.size()), JobsNum);

    std::set<int> Used;
    for (int i = 0; i < JobsNum; ++i)
    {
        EXPECT_GE(Assign[i], 0) << "job " << i << " maps to a negative worker";
        EXPECT_LT(Assign[i], WorkersNum) << "job " << i << " maps to an out-of-range worker";
        Used.insert(Assign[i]);
    }
    // every job must be matched to a distinct worker
    EXPECT_EQ(static_cast<int>(Used.size()), JobsNum) << "two jobs share the same worker";
}

// Check the solver result against the brute-force optimum: it must be a valid
// assignment whose total cost equals the known minimum.
template <typename CostType>
void ExpectOptimal(const TK::THungarianSolver<CostType>& Solver,
                   const std::vector<int>& Assign)
{
    ValidateAssignment(Assign, Solver.JobsNum, Solver.WorkersNum);

    const CostType Got = TotalCostOf(Solver.Costs, Assign, Solver.WorkersNum);
    const CostType Want = BruteForceMinCost(Solver.Costs, Solver.JobsNum, Solver.WorkersNum);
    EXPECT_EQ(Got, Want) << "solver did not reach the minimum total cost";
}

// ============================================================================
// 1. Trivial sizes
// ============================================================================

TEST(HungarianSolver, SingleJobSingleWorker)
{
    IntSolver S;
    S.JobsNum = 1;
    S.WorkersNum = 1;
    S.Costs = {7};

    const auto& Assign = S.Solve();

    ASSERT_EQ(Assign.size(), 1u);
    EXPECT_EQ(Assign[0], 0);
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 7);
}

TEST(HungarianSolver, SingleJobPicksCheapestWorker)
{
    IntSolver S;
    S.JobsNum = 1;
    S.WorkersNum = 5;
    S.Costs = {9, 3, 8, 1, 4}; // minimum is index 3

    const auto& Assign = S.Solve();

    ASSERT_EQ(Assign.size(), 1u);
    EXPECT_EQ(Assign[0], 3);
    ExpectOptimal(S, Assign);
}

// ============================================================================
// 2. Square matrices
// ============================================================================

TEST(HungarianSolver, IdentityMatrixGivesDiagonal)
{
    // cost[i][i] = 0, everything else huge => unique optimum is the diagonal.
    IntSolver S;
    S.JobsNum = 3;
    S.WorkersNum = 3;
    S.Costs = {
        0,   100, 100,
        100, 0,   100,
        100, 100, 0
    };

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    EXPECT_EQ(Assign[0], 0);
    EXPECT_EQ(Assign[1], 1);
    EXPECT_EQ(Assign[2], 2);
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 0);
}

TEST(HungarianSolver, AntiDiagonalAssignment)
{
    // unique optimum is the anti-diagonal: job i -> worker (n-1-i).
    IntSolver S;
    S.JobsNum = 3;
    S.WorkersNum = 3;
    S.Costs = {
        10, 10, 1,
        10, 1,  10,
        1,   10, 10
    };

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    EXPECT_EQ(Assign[0], 2);
    EXPECT_EQ(Assign[1], 1);
    EXPECT_EQ(Assign[2], 0);
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 3);
}

TEST(HungarianSolver, AllEqualCosts)
{
    // Any matching is optimal; we only validate structure and total cost.
    IntSolver S;
    S.JobsNum = 4;
    S.WorkersNum = 4;
    S.Costs = std::vector<int>(16, 5);

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 5 * 4);
}

TEST(HungarianSolver, KnownSquareOptimum)
{
    IntSolver S;
    S.JobsNum = 3;
    S.WorkersNum = 3;
    S.Costs = {
        4, 1, 3,
        2, 0, 5,
        3, 2, 2
    };

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    // minimum is 1 + 2 + 2 = 5 (job0->1, job1->0, job2->2)
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 5);
}

// ============================================================================
// 3. Rectangular: more workers than jobs
// ============================================================================

TEST(HungarianSolver, RectangularMoreWorkersThanJobs)
{
    IntSolver S;
    S.JobsNum = 2;
    S.WorkersNum = 4;
    S.Costs = {
        10, 8, 2, 9,
        1,  7, 3, 4
    };

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    // best: job0->2 (2), job1->0 (1) = 3
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 3);
}

TEST(HungarianSolver, RectangularPrefersCheapColumns)
{
    IntSolver S;
    S.JobsNum = 3;
    S.WorkersNum = 6;
    // three cheap columns (3,4,5) form the diagonal-ish optimum.
    S.Costs = {
        9, 9, 9, 0, 9, 9,
        9, 9, 9, 9, 0, 9,
        9, 9, 9, 9, 9, 0
    };

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    EXPECT_EQ(Assign[0], 3);
    EXPECT_EQ(Assign[1], 4);
    EXPECT_EQ(Assign[2], 5);
    EXPECT_EQ(TotalCostOf<int>(S.Costs, Assign, S.WorkersNum), 0);
}

// ============================================================================
// 4. int64_t instantiation
// ============================================================================

TEST(HungarianSolver, Int64SquareOptimum)
{
    Int64Solver S;
    S.JobsNum = 3;
    S.WorkersNum = 3;
    S.Costs = {
        std::int64_t(1),  std::int64_t(2),  std::int64_t(3),
        std::int64_t(6),  std::int64_t(5),  std::int64_t(4),
        std::int64_t(1),  std::int64_t(1),  std::int64_t(1)
    };

    const auto& Assign = S.Solve();

    ValidateAssignment(Assign, S.JobsNum, S.WorkersNum);
    // brute force handles int64 too
    EXPECT_EQ(TotalCostOf<std::int64_t>(S.Costs, Assign, S.WorkersNum),
              BruteForceMinCost(S.Costs, S.JobsNum, S.WorkersNum));
}

TEST(HungarianSolver, Int64LargeValues)
{
    // values well below the int64 infinity sentinel.
    Int64Solver S;
    S.JobsNum = 2;
    S.WorkersNum = 3;
    S.Costs = {
        std::int64_t(1000000000), std::int64_t(1),          std::int64_t(2000000000),
        std::int64_t(2),          std::int64_t(1000000000), std::int64_t(3000000000)
    };

    const auto& Assign = S.Solve();

    ExpectOptimal(S, Assign);
    // best: job0->1 (1), job1->0 (2) = 3
    EXPECT_EQ(TotalCostOf<std::int64_t>(S.Costs, Assign, S.WorkersNum), 3);
}

// ============================================================================
// 5. Re-solve: calling Solve() twice is idempotent
// ============================================================================

TEST(HungarianSolver, SolveTwiceSameResult)
{
    IntSolver S;
    S.JobsNum = 3;
    S.WorkersNum = 3;
    S.Costs = {
        4, 1, 3,
        2, 0, 5,
        3, 2, 2
    };

    const auto& First = S.Solve();
    std::vector<int> Snapshot(First.begin(), First.end());

    const auto& Second = S.Solve();

    EXPECT_EQ(Second.size(), Snapshot.size());
    for (int i = 0; i < S.JobsNum; ++i)
        EXPECT_EQ(Second[i], Snapshot[i]) << "mismatch on re-solve at job " << i;
    ExpectOptimal(S, Second);
}

// ============================================================================
// 6. Randomized stress test vs brute force
// ============================================================================

TEST(HungarianSolver, RandomStressMatchesBruteForce)
{
    // Deterministic seed for reproducibility.
    std::mt19937 Rng(12345);

    for (int Iter = 0; Iter < 300; ++Iter)
    {
        // keep workers small so brute force (m!) stays cheap.
        std::uniform_int_distribution<int> SizeDist(1, 6);
        int WorkersNum = SizeDist(Rng);
        int JobsNum = std::uniform_int_distribution<int>(1, WorkersNum)(Rng);

        // keep costs small enough to never approach the int infinity sentinel.
        std::uniform_int_distribution<int> CostDist(0, 50);

        IntSolver S;
        S.JobsNum = JobsNum;
        S.WorkersNum = WorkersNum;
        S.Costs.resize(JobsNum * WorkersNum);
        for (auto& C : S.Costs)
            C = CostDist(Rng);

        const auto& Assign = S.Solve();

        // Always validate the matching; check optimality against brute force.
        ValidateAssignment(Assign, JobsNum, WorkersNum);
        const int Got = TotalCostOf<int>(S.Costs, Assign, WorkersNum);
        const int Want = BruteForceMinCost(S.Costs, JobsNum, WorkersNum);
        EXPECT_EQ(Got, Want) << "iteration " << Iter
                             << " (jobs=" << JobsNum << ", workers=" << WorkersNum << ")";
    }
}

TEST(HungarianSolver, RandomStressInt64MatchesBruteForce)
{
    std::mt19937 Rng(98765);

    for (int Iter = 0; Iter < 200; ++Iter)
    {
        std::uniform_int_distribution<int> SizeDist(1, 6);
        int WorkersNum = SizeDist(Rng);
        int JobsNum = std::uniform_int_distribution<int>(1, WorkersNum)(Rng);
        std::uniform_int_distribution<int> CostDist(0, 100);

        Int64Solver S;
        S.JobsNum = JobsNum;
        S.WorkersNum = WorkersNum;
        S.Costs.resize(JobsNum * WorkersNum);
        for (auto& C : S.Costs)
            C = static_cast<std::int64_t>(CostDist(Rng));

        const auto& Assign = S.Solve();

        ValidateAssignment(Assign, JobsNum, WorkersNum);
        const std::int64_t Got = TotalCostOf<std::int64_t>(S.Costs, Assign, WorkersNum);
        const std::int64_t Want = BruteForceMinCost(S.Costs, JobsNum, WorkersNum);
        EXPECT_EQ(Got, Want) << "int64 iteration " << Iter
                             << " (jobs=" << JobsNum << ", workers=" << WorkersNum << ")";
    }
}

// ============================================================================
// 7. Fuzz test with a fresh seed each run (repeatable via env var)
// ----------------------------------------------------------------------------
// Unlike the fixed-seed tests above, this one draws a *new* random seed on
// every execution so that repeatedly running the binary keeps exploring new
// inputs. To reproduce a failing run, set the seed printed in the failure
// message:
//
//     TINKER_HUNGARIAN_SEED=<seed>      # also fixes cost range:
//     TINKER_HUNGARIAN_ITERS=<count>    # optional, default 2000
// ============================================================================

// Parse a non-negative decimal env var; returns fallback on missing/garbage.
static unsigned long ParseEnvUL(const char* Name, unsigned long Fallback)
{
    const char* Raw = std::getenv(Name);
    if (!Raw || !*Raw)
        return Fallback;
    try
    {
        return std::stoul(Raw);
    }
    catch (...)
    {
        return Fallback;
    }
}

TEST(HungarianSolver, FuzzFreshSeedEachRun)
{
    // Default: a fresh, unpredictable seed from the OS each run. Override with
    // TINKER_HUNGARIAN_SEED=<n> to replay a specific run.
    unsigned long Seed;
    const char* EnvSeed = std::getenv("TINKER_HUNGARIAN_SEED");
    if (EnvSeed && *EnvSeed)
    {
        Seed = ParseEnvUL("TINKER_HUNGARIAN_SEED", 0);
    }
    else
    {
        std::random_device RD;
        Seed = static_cast<unsigned long>(RD());
    }

    // A single run exercises many cases; bump the count for longer fuzzing.
    const unsigned long ITERS = ParseEnvUL("TINKER_HUNGARIAN_ITERS", 2000);

    std::mt19937 Rng(static_cast<std::mt19937::result_type>(Seed));

    // keep workers small so brute force (m!) stays cheap.
    std::uniform_int_distribution<int> SizeDist(1, 7);
    // keep costs well below the int infinity sentinel 0x3f3f3f3f.
    std::uniform_int_distribution<int> CostDist(0, 100);

    for (unsigned long Iter = 0; Iter < ITERS; ++Iter)
    {
        int WorkersNum = SizeDist(Rng);
        int JobsNum = std::uniform_int_distribution<int>(1, WorkersNum)(Rng);

        IntSolver S;
        S.JobsNum = JobsNum;
        S.WorkersNum = WorkersNum;
        S.Costs.resize(JobsNum * WorkersNum);
        for (auto& C : S.Costs)
            C = CostDist(Rng);

        const auto& Assign = S.Solve();

        // On failure, print enough to reproduce this exact run.
        ::testing::ScopedTrace Trace(__FILE__, __LINE__,
            ("seed=" + std::to_string(Seed) +
             " iter=" + std::to_string(Iter) +
             " jobs=" + std::to_string(JobsNum) +
             " workers=" + std::to_string(WorkersNum)).c_str());

        ValidateAssignment(Assign, JobsNum, WorkersNum);
        const int Got = TotalCostOf<int>(S.Costs, Assign, WorkersNum);
        const int Want = BruteForceMinCost(S.Costs, JobsNum, WorkersNum);
        EXPECT_EQ(Got, Want);
    }
}
