/**
 * @file tier_worker.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the worker module for the Loopy Tier Solver.
 * @version 1.2.1
 * @date 2024-02-29
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/solvers/tier_solver/tier_worker.h"

#include <assert.h>   // assert
#include <limits.h>   // UCHAR_MAX
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, uint64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, free
#include <string.h>   // memcpy

#include "core/constants.h"
#include "core/db/db_manager.h"
#include "core/solvers/tier_solver/frontier.h"
#include "core/solvers/tier_solver/reverse_graph.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "libs/mt19937/mt19937-64.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>        // OpenMP pragmas
#include <stdatomic.h>  // atomic_uchar, atomic functions, memory_order_relaxed
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_PARALLEL PRAGMA(omp parallel)
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp for schedule(dynamic, k))
#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp parallel for schedule(dynamic, k))

#else  // _OPENMP not defined, the following macros do nothing.
#define PRAGMA
#define PRAGMA_OMP_PARALLEL
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k)
#define PRAGMA_OMP_PARALLEL_FOR
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k)
#endif  // _OPENMP

#ifdef USE_MPI
#include <unistd.h>  // sleep

#include "core/solvers/tier_solver/tier_mpi.h"
#endif  // USE_MPI

// Note on multithreading:
//   Be careful that "if (!condition) success = false;" is not equivalent to
//   "success &= condition" or "success = condition". The former creates a race
//   condition whereas the latter may overwrite an already failing result.

// Copy of the API functions from tier_manager. Cannot use a reference here
// because we need to create/modify some of the functions.
static TierSolverApi current_api;

static int64_t current_db_chunk_size;

// A frontier array will be created for each possible remoteness.
static const int kFrontierSize = kRemotenessMax + 1;

static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.
// Array of child tiers with this_tier appended to the back.
static TierArray child_tiers;

// A frontier contains solved but unprocessed positions.
static Frontier *win_frontiers;   // Winning frontiers for each thread.
static Frontier *lose_frontiers;  // Losing frontiers for each thread.
static Frontier *tie_frontiers;   // Tying frontiers for each thread.

// Number of undecided child positions array (malloc'ed and owned by the
// TierWorkerSolve function). Note that we are assuming the number of children
// of ANY position is no more than 254. This allows us to use an unsigned 8-bit
// integer to save memory. If this assumption no longer holds for any new
// games in the future, the programmer should change this type to a wider
// integer type such as int16_t.
#ifdef _OPENMP
static atomic_uchar *num_undecided_children = NULL;
#else   // _OPENMP not defined
static unsigned char *num_undecided_children = NULL;
#endif  // _OPENMP

// Cached reverse position graph of the current tier. This is only initialized
// if the game does not implement Retrograde Analysis.
ReverseGraph reverse_graph;
// The reverse graph is used if the Retrograde Analysis is turned off.
static bool use_reverse_graph = false;

static int num_threads;  // Number of threads available.

static bool Step0Initialize(Tier tier);
static bool Step0_0InitFrontiers(int dividers_size);
static int GetThreadId(void);
static PositionArray TierGetCanonicalParentPositionsFromReverseGraph(
    TierPosition child, Tier parent_tier);

static bool Step1LoadChildren(void);
static bool IsCanonicalTier(Tier tier);
static bool Step1_0LoadCanonicalTier(int child_index);
static bool Step1_1LoadNonCanonicalTier(int child_index);
static bool CheckAndLoadFrontier(int child_index, int64_t position, Value value,
                                 int remoteness, int tid);

static bool Step2SetupSolverArrays(void);

static bool Step3ScanTier(void);
static bool IsCanonicalPosition(Position position);
static int Step3_0CountChildren(Position position);

static bool Step4PushFrontierUp(void);
static bool PushFrontierHelper(
    Frontier *frontiers, int remoteness,
    bool (*ProcessPosition)(int remoteness, TierPosition tier_position));
static int64_t *MakeFrontierOffsets(const Frontier *frontiers, int remoteness);
static void UpdateFrontierAndChildTierIds(int64_t i, const Frontier *frontiers,
                                          int *frontier_id, int *child_index,
                                          int remoteness,
                                          const int64_t *frontier_offsets);

static bool ProcessLosePosition(int remoteness, TierPosition tier_position);
static bool ProcessWinPosition(int remoteness, TierPosition tier_position);
static bool ProcessTiePosition(int remoteness, TierPosition tier_position);

static void Step5MarkDrawPositions(void);
static void Step6SaveValues(void);
static bool CompareDb(void);
static void Step7Cleanup(void);
static void DestroyFrontiers(void);

static bool TestShouldSkip(Tier tier, Position position);
static int TestChildPositions(Tier tier, Position position);
static int TestChildToParentMatching(Tier tier, Position position);
static int TestParentToChildMatching(Tier tier, Position position,
                                     const TierArray *parent_tiers);
static int TestPrintError(Tier tier, Position position);

// -----------------------------------------------------------------------------

void TierWorkerInit(const TierSolverApi *api, int64_t db_chunk_size) {
    memcpy(&current_api, api, sizeof(current_api));
    current_db_chunk_size = db_chunk_size;
    assert(current_db_chunk_size > 0);
}

int TierWorkerSolve(Tier tier, bool force, bool compare, bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        ret = kNoError;  // Success.
        goto _bailout;
    }

    /* Solver main algorithm. */
    if (!Step0Initialize(tier)) goto _bailout;
    if (!Step1LoadChildren()) goto _bailout;
    if (!Step2SetupSolverArrays()) goto _bailout;
    if (!Step3ScanTier()) goto _bailout;
    if (!Step4PushFrontierUp()) goto _bailout;
    Step5MarkDrawPositions();
    Step6SaveValues();
    if (compare && !CompareDb()) goto _bailout;
    if (solved != NULL) *solved = true;
    ret = kNoError;  // Success.

_bailout:
    Step7Cleanup();
    return ret;
}

#ifdef USE_MPI
int TierWorkerMpiServe(void) {
    TierMpiWorkerSendCheck();
    while (true) {
        TierMpiManagerMessage msg;
        TierMpiWorkerRecv(&msg);

        if (msg.command == kTierMpiCommandSleep) {
            // No work to do. Wait for one second and check again.
            sleep(1);
            TierMpiWorkerSendCheck();
        } else if (msg.command == kTierMpiCommandTerminate) {
            break;
        } else {
            bool force = (msg.command == kTierMpiCommandForceSolve);
            bool solved;
            int error = TierWorkerSolve(msg.tier, force, false, &solved);
            if (error != kNoError) {
                TierMpiWorkerSendReportError(error);
            } else if (solved) {
                TierMpiWorkerSendReportSolved();
            } else {
                TierMpiWorkerSendReportLoaded();
            }
        }
    }

    return kNoError;
}
#endif  // USE_MPI

static int TestTierSymmetryRemoval(Tier tier, Position position,
                                   Tier canonical_tier) {
    Position (*ApplySymm)(TierPosition, Tier) =
        current_api.GetPositionInSymmetricTier;

    TierPosition self = {.tier = tier, .position = position};
    TierPosition symm = {.tier = canonical_tier};
    symm.position = ApplySymm(self, canonical_tier);

    // Test if getting symmetric position from the same tier returns the
    // position itself.
    Position self_in_self_tier = ApplySymm(self, self.tier);
    Position symm_in_symm_tier = ApplySymm(symm, symm.tier);
    if (self_in_self_tier != self.position) {
        return kTierSolverTestTierSymmetrySelfMappingError;
    } else if (symm_in_symm_tier != symm.position) {
        return kTierSolverTestTierSymmetrySelfMappingError;
    }

    // Skip the next test if both tiers are the same.
    if (tier == canonical_tier) return kTierSolverTestNoError;

    // Test if applying the symmetry twice returns the same position.
    Position self_in_symm_tier = ApplySymm(self, symm.tier);
    Position symm_in_self_tier = ApplySymm(symm, self.tier);
    TierPosition self_in_symm_tier_tier_position = {
        .tier = symm.tier, .position = self_in_symm_tier};
    TierPosition symm_in_self_tier_tier_position = {
        .tier = self.tier, .position = symm_in_self_tier};
    Position new_self = ApplySymm(self_in_symm_tier_tier_position, self.tier);
    Position new_symm = ApplySymm(symm_in_self_tier_tier_position, symm.tier);
    if (new_self != self.position) {
        return kTierSolverTestTierSymmetryInconsistentError;
    } else if (new_symm != symm.position) {
        return kTierSolverTestTierSymmetryInconsistentError;
    }

    return kTierSolverTestNoError;
}

int TierWorkerTest(Tier tier, const TierArray *parent_tiers, long seed) {
    static const int64_t kTestSizeMax = 1000;
    init_genrand64(seed);

    int64_t tier_size = current_api.GetTierSize(tier);
    bool random_test = tier_size > kTestSizeMax;
    int64_t test_size = random_test ? kTestSizeMax : tier_size;
    Tier canonical_tier = current_api.GetCanonicalTier(tier);

    for (int64_t i = 0; i < test_size; ++i) {
        Position position = random_test ? genrand64_int63() % tier_size : i;
        if (TestShouldSkip(tier, position)) continue;

        // Check tier symmetry removal implementation.
        int error = TestTierSymmetryRemoval(tier, position, canonical_tier);
        if (error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            return error;
        }

        // Check if all child positions are legal.
        error = TestChildPositions(tier, position);
        if (error != kTierSolverTestNoError) {
            TestPrintError(tier, position);
            return error;
        }

        // Perform the following tests only if current game variant implements
        // its own GetCanonicalParentPositions.
        if (current_api.GetCanonicalParentPositions != NULL) {
            // Check if all child positions of the current position has the
            // current position as one of their parents.
            error = TestChildToParentMatching(tier, position);
            if (error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                return error;
            }

            // Check if all parent positions of the current position has the
            // current position as one of their children.
            error = TestParentToChildMatching(tier, position, parent_tiers);
            if (error != kTierSolverTestNoError) {
                TestPrintError(tier, position);
                return error;
            }
        }
    }

    return kTierSolverTestNoError;
}

// -----------------------------------------------------------------------------

static bool Step0Initialize(Tier tier) {
    // Initialize child tier array.
    this_tier = tier;
    child_tiers = current_api.GetChildTiers(this_tier);
    if (child_tiers.size == kIllegalSize) return false;

    // Initialize reverse graph without this_tier in the child_tiers array.
    use_reverse_graph = (current_api.GetCanonicalParentPositions == NULL);
    if (use_reverse_graph) {
        bool success = ReverseGraphInit(&reverse_graph, &child_tiers, this_tier,
                                        current_api.GetTierSize);
        if (!success) return false;
        current_api.GetCanonicalParentPositions =
            &TierGetCanonicalParentPositionsFromReverseGraph;
    }

    // From this point on, child_tiers will also contain this_tier.
    TierArrayAppend(&child_tiers, this_tier);

    // Initialize frontiers with size to hold all child tiers and this tier.
    if (!Step0_0InitFrontiers(child_tiers.size)) return false;

    // Non-memory-allocating initializations.
    this_tier_size = current_api.GetTierSize(tier);
    return true;
}

static bool Step0_0InitFrontiers(int dividers_size) {
#ifdef _OPENMP
    num_threads = omp_get_max_threads();
#else   // _OPENMP not defined.
    num_threads = 1;
#endif  // _OPENMP
    win_frontiers = (Frontier *)malloc(num_threads * sizeof(Frontier));
    lose_frontiers = (Frontier *)malloc(num_threads * sizeof(Frontier));
    tie_frontiers = (Frontier *)malloc(num_threads * sizeof(Frontier));
    if (!win_frontiers || !lose_frontiers || !tie_frontiers) {
        return false;
    }

    bool success = true;
    for (int i = 0; i < num_threads; ++i) {
        success &=
            FrontierInit(&win_frontiers[i], kFrontierSize, dividers_size);
        success &=
            FrontierInit(&lose_frontiers[i], kFrontierSize, dividers_size);
        success &=
            FrontierInit(&tie_frontiers[i], kFrontierSize, dividers_size);
    }

    return success;
}

static int GetThreadId(void) {
#ifdef _OPENMP
    return omp_get_thread_num();
#else   // _OPENMP not defined, thread 0 is the only available thread.
    return 0;
#endif  // _OPENMP
}

static PositionArray TierGetCanonicalParentPositionsFromReverseGraph(
    TierPosition child, Tier parent_tier) {
    // Unused, since all children were generated by positions in this_tier.
    (void)parent_tier;
    return ReverseGraphPopParentsOf(&reverse_graph, child);
}

/**
 * @brief Load all non-drawing positions from all child tiers into frontier.
 */
static bool Step1LoadChildren(void) {
    // Child tiers must be processed sequentially, otherwise the frontier
    // dividers wouldn't work.
    int num_child_tiers = child_tiers.size - 1;
    for (int child_index = 0; child_index < num_child_tiers; ++child_index) {
        // Load child tier from disk.
        bool childIsCanonical = IsCanonicalTier(child_tiers.array[child_index]);
        bool success;
        if (childIsCanonical) {
            success = Step1_0LoadCanonicalTier(child_index);
        } else {
            success = Step1_1LoadNonCanonicalTier(child_index);
        }
        if (!success) return false;
    }
    return true;
}

static bool IsCanonicalTier(Tier tier) {
    return current_api.GetCanonicalTier(tier) == tier;
}

static bool Step1_0LoadCanonicalTier(int child_index) {
    Tier child_tier = child_tiers.array[child_index];

    // Scan child tier and load non-drawing positions into frontier.
    int64_t child_tier_size = current_api.GetTierSize(child_tier);
    bool success = true;

    PRAGMA_OMP_PARALLEL {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition child_tier_position = {.tier = child_tier};
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(current_db_chunk_size)
        for (Position position = 0; position < child_tier_size; ++position) {
            child_tier_position.position = position;
            Value value = DbManagerProbeValue(&probe, child_tier_position);
            int remoteness =
                DbManagerProbeRemoteness(&probe, child_tier_position);
            if (!CheckAndLoadFrontier(child_index, position, value, remoteness,
                                      tid)) {
                success = false;
            }
        }
        DbManagerProbeDestroy(&probe);
    }

    return success;
}

static bool Step1_1LoadNonCanonicalTier(int child_index) {
    Tier original_tier = child_tiers.array[child_index];
    Tier canonical_tier = current_api.GetCanonicalTier(original_tier);

    // Scan child tier and load winning/losing positions into frontier.
    int64_t child_tier_size = current_api.GetTierSize(canonical_tier);
    bool success = true;

    PRAGMA_OMP_PARALLEL {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition canonical_tier_position = {.tier = canonical_tier};
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(current_db_chunk_size)
        for (int64_t position = 0; position < child_tier_size; ++position) {
            canonical_tier_position.position = position;
            Value value = DbManagerProbeValue(&probe, canonical_tier_position);

            // No need to convert hash if position does not need to be loaded.
            if (value == kUndecided || value == kDraw) continue;

            int remoteness =
                DbManagerProbeRemoteness(&probe, canonical_tier_position);
            Position position_in_noncanonical_tier =
                current_api.GetPositionInSymmetricTier(canonical_tier_position,
                                                       original_tier);
            if (!CheckAndLoadFrontier(child_index,
                                      position_in_noncanonical_tier, value,
                                      remoteness, tid)) {
                success = false;
            }
        }
        DbManagerProbeDestroy(&probe);
    }

    return success;
}

static bool CheckAndLoadFrontier(int child_index, int64_t position, Value value,
                                 int remoteness, int tid) {
    if (remoteness < 0) return false;  // Error probing remoteness.
    if (value == kUndecided || value == kDraw) return true;
    Frontier *dest = NULL;
    switch (value) {
        case kUndecided:
        case kDraw:
            return true;

        case kWin:
            dest = &win_frontiers[tid];
            break;

        case kLose:
            dest = &lose_frontiers[tid];
            break;

        case kTie:
            dest = &tie_frontiers[tid];
            break;

        default:
            return false;  // Error probing value.
    }
    return FrontierAdd(dest, position, remoteness, child_index);
}

/**
 * @brief Initializes database and number of undecided children array.
 */
static bool Step2SetupSolverArrays(void) {
    int error = DbManagerCreateSolvingTier(this_tier,
                                           current_api.GetTierSize(this_tier));
    if (error != 0) return false;

#ifdef _OPENMP
    num_undecided_children =
        (atomic_uchar *)calloc(this_tier_size, sizeof(atomic_uchar));
#else   // _OPENMP not defined
    num_undecided_children =
        (unsigned char *)calloc(this_tier_size, sizeof(unsigned char));
#endif  // _OPENMP
    return (num_undecided_children != NULL);
}

/**
 * @brief Counts the number of children of all positions in current tier and
 * loads primitive positions into frontier.
 */
static bool Step3ScanTier(void) {
    bool success = true;

    PRAGMA_OMP_PARALLEL {
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (Position position = 0; position < this_tier_size; ++position) {
            TierPosition tier_position = {.tier = this_tier,
                                          .position = position};

            // Skip illegal positions and non-canonical positions.
            if (!current_api.IsLegalPosition(tier_position) ||
                !IsCanonicalPosition(position)) {
                num_undecided_children[position] = 0;
                continue;
            }

            Value value = current_api.Primitive(tier_position);
            if (value != kUndecided) {  // If tier_position is primitive...
                // Set its value immediately and push it into the frontier.
                DbManagerSetValue(position, value);
                DbManagerSetRemoteness(position, 0);
                int this_tier_index = child_tiers.size - 1;
                if (!CheckAndLoadFrontier(this_tier_index, position, value, 0,
                                          tid)) {
                    success = false;
                }
                num_undecided_children[position] = 0;
                continue;
            }  // Execute the following lines if tier_position is not primitive.
            int num_children = Step3_0CountChildren(position);
            if (num_children <= 0)
                success = false;  // Either OOM or no children.
            num_undecided_children[position] = num_children;
        }
    }

    for (int i = 0; i < num_threads; ++i) {
        FrontierAccumulateDividers(&win_frontiers[i]);
        FrontierAccumulateDividers(&lose_frontiers[i]);
        FrontierAccumulateDividers(&tie_frontiers[i]);
    }

    return success;
}

static bool IsCanonicalPosition(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    return current_api.GetCanonicalPosition(tier_position) == position;
}

static int Step3_0CountChildren(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    if (!use_reverse_graph) {
        return current_api.GetNumberOfCanonicalChildPositions(tier_position);
    }
    // Else, count children manually and add position as their parent in the
    // reverse graph.
    TierPositionArray children =
        current_api.GetCanonicalChildPositions(tier_position);

    for (int64_t i = 0; i < children.size; ++i) {
        if (!ReverseGraphAdd(&reverse_graph, children.array[i], position)) {
            return -1;
        }
    }
    int num_children = (int)children.size;
    TierPositionArrayDestroy(&children);
    return num_children;
}

/**
 * @brief Pushes frontier up.
 */
static bool Step4PushFrontierUp(void) {
    // Process winning and losing positions first.
    // Remotenesses must be processed sequentially.
    for (int remoteness = 0; remoteness < kFrontierSize; ++remoteness) {
        if (!PushFrontierHelper(lose_frontiers, remoteness,
                                &ProcessLosePosition)) {
            return false;
        } else if (!PushFrontierHelper(win_frontiers, remoteness,
                                       &ProcessWinPosition)) {
            return false;
        }
    }

    // Then move on to tying positions.
    for (int remoteness = 0; remoteness < kFrontierSize; ++remoteness) {
        if (!PushFrontierHelper(tie_frontiers, remoteness, &ProcessTiePosition))
            return false;
    }
    DestroyFrontiers();
    TierArrayDestroy(&child_tiers);
    ReverseGraphDestroy(&reverse_graph);
    return true;
}

/**
 * @details The algorithm is as follows: first count the total number N of
 * positions that need to be processed and then run a parallel for loop that
 * ranges from 0 to N-1 to process each position. In order for threads to figure
 * out which tier a position belongs to, it must first figure out which frontier
 * that position was taken from, and then use the corresponding "dividers" array
 * together with the child_tiers array to figure out which tier that position
 * is from.
 *
 * This function first inspects all positions in the frontier (multiple
 * Frontier instances if multithreading) at the given remoteness that needs to
 * be processed, and creates an array of offsets that allows us to determine
 * which Frontier instance a position belongs to. Then it uses the helper
 * function UpdateFrontierAndChildTierIds to figure out which frontier and which
 * child tier a position was from. The helper function is designed to take in
 * the old values of frontier_id and child_index as hints on where to begin
 * searching. This allows us to not start the search at index 0 for every
 * position. However, it also means that we are assuming an order of processing
 * within each tier. If the order is random, the hints will not work correctly.
 */
static bool PushFrontierHelper(
    Frontier *frontiers, int remoteness,
    bool (*ProcessPosition)(int remoteness, TierPosition tier_position)) {
    //
    int64_t *frontier_offsets = MakeFrontierOffsets(frontiers, remoteness);
    if (!frontier_offsets) return false;

    bool success = true;
    PRAGMA_OMP_PARALLEL {
        int frontier_id = 0, child_index = 0;
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(16)
        for (int64_t i = 0; i < frontier_offsets[num_threads]; ++i) {
            UpdateFrontierAndChildTierIds(i, frontiers, &frontier_id,
                                          &child_index, remoteness,
                                          frontier_offsets);
            int64_t index_in_frontier = i - frontier_offsets[frontier_id];
            TierPosition tier_position = {
                .tier = child_tiers.array[child_index],
                .position = FrontierGetPosition(&frontiers[frontier_id],
                                                remoteness, index_in_frontier),
            };
            if (!ProcessPosition(remoteness, tier_position)) success = false;
        }
    }

    // Free current remoteness from all frontiers.
    for (int i = 0; i < num_threads; ++i) {
        FrontierFreeRemoteness(&frontiers[i], remoteness);
    }
    free(frontier_offsets);
    frontier_offsets = NULL;

    return success;
}

static int64_t *MakeFrontierOffsets(const Frontier *frontiers, int remoteness) {
    int64_t *frontier_offsets =
        (int64_t *)malloc((num_threads + 1) * sizeof(int64_t));
    if (frontier_offsets == NULL) return NULL;

    frontier_offsets[0] = 0;
    for (int i = 1; i <= num_threads; ++i) {
        frontier_offsets[i] =
            frontier_offsets[i - 1] + frontiers[i - 1].buckets[remoteness].size;
    }

    return frontier_offsets;
}

// This function assumes frontier_id and child_index passed in corresponds to
// a chunk of positions that either contains the i-th position in the array of
// all positions, or corresponds to a chunk that comes later in the array.
static void UpdateFrontierAndChildTierIds(int64_t i, const Frontier *frontiers,
                                          int *frontier_id, int *child_index,
                                          int remoteness,
                                          const int64_t *frontier_offsets) {
    while (i >= frontier_offsets[*frontier_id + 1]) {
        ++(*frontier_id);
        *child_index = 0;
    }
    int64_t index_in_frontier = i - frontier_offsets[*frontier_id];
    while (index_in_frontier >=
           frontiers[*frontier_id].dividers[remoteness][*child_index]) {
        ++(*child_index);
    }
}

// This function is called within a OpenMP parallel region.
static bool ProcessLoseOrTiePosition(int remoteness, TierPosition tier_position,
                                     bool processing_lose) {
    PositionArray parents =
        current_api.GetCanonicalParentPositions(tier_position, this_tier);
    if (parents.size < 0) {  // OOM.
        TierArrayDestroy(&parents);
        return false;
    }

    int tid = GetThreadId();
    Value value = processing_lose ? kWin : kTie;
    Frontier *frontier =
        processing_lose ? &win_frontiers[tid] : &tie_frontiers[tid];
    for (int64_t i = 0; i < parents.size; ++i) {
#ifdef _OPENMP
        // Atomically fetch num_undecided_children[parents.array[i]] into
        // child_remaining and set it to zero.
        unsigned char child_remaining = atomic_exchange_explicit(
            &num_undecided_children[parents.array[i]], 0, memory_order_relaxed);
#else                                        // _OPENMP not defined
        unsigned char child_remaining =
            num_undecided_children[parents.array[i]];
        num_undecided_children[parents.array[i]] = 0;
#endif                                       // _OPENMP
        if (child_remaining == 0) continue;  // Parent already solved.

        // All parents are win/tie in (remoteness + 1) positions.
        DbManagerSetValue(parents.array[i], value);
        DbManagerSetRemoteness(parents.array[i], remoteness + 1);
        int this_tier_index = child_tiers.size - 1;
        bool success = FrontierAdd(frontier, parents.array[i], remoteness + 1,
                                   this_tier_index);
        if (!success) {  // OOM.
            TierArrayDestroy(&parents);
            return false;
        }
    }
    TierArrayDestroy(&parents);
    return true;
}

static bool ProcessLosePosition(int remoteness, TierPosition tier_position) {
    return ProcessLoseOrTiePosition(remoteness, tier_position, true);
}

#ifdef _OPENMP
/**
 * @brief Atomically decrements the content of OBJ, which is of type unsigned
 * char, if and only if it was greater than 0, and returns the original value.
 * If multiple threads call this function on the same OBJ at the same time, the
 * value returned is guaranteed to be unique for each thread if no threads are
 * performing other operations on OBJ.
 *
 * @note This is a helper function that is only used by ProcessWinPosition.
 *
 * @param obj Atomic unsigned char object to be decremented.
 * @return Original value of OBJ.
 */
static unsigned char DecrementIfNonZero(atomic_uchar *obj) {
    unsigned char current_value =
        atomic_load_explicit(obj, memory_order_relaxed);
    while (current_value != 0) {
        // This function will set OBJ to current_value - 1 if OBJ is still equal
        // to current_value. Otherwise, it updates current_value to the new
        // value of OBJ and returns false.
        bool success = atomic_compare_exchange_strong_explicit(
            obj, &current_value, current_value - 1, memory_order_relaxed,
            memory_order_relaxed);
        // If decrement was successful, quit and return the original value of
        // OBJ that was swapped out.
        if (success) return current_value;
        // Otherwise, we keep looping until either a decrement was successfully
        // completed, or OBJ is decremented to zero by some other thread.
    }

    return 0;
}
#endif  // _OPENMP

// This function is called within a OpenMP parallel region.
static bool ProcessWinPosition(int remoteness, TierPosition tier_position) {
    PositionArray parents =
        current_api.GetCanonicalParentPositions(tier_position, this_tier);
    if (parents.size < 0) {  // OOM.
        PositionArrayDestroy(&parents);
        return false;
    }

    int tid = GetThreadId();
    for (int64_t i = 0; i < parents.size; ++i) {
#ifdef _OPENMP
        unsigned char child_remaining =
            DecrementIfNonZero(&num_undecided_children[parents.array[i]]);
#else   // _OPENMP not defined
        // If this parent has been solved already, skip it.
        if (num_undecided_children[parents.array[i]] == 0) continue;
        // Must perform the above check before decrementing to prevent overflow.
        unsigned char child_remaining =
            num_undecided_children[parents.array[i]]--;
#endif  // _OPENMP
        // If this child position is the last undecided child of parent
        // position, mark parent as lose in (childRmt + 1).
        if (child_remaining == 1) {
            DbManagerSetValue(parents.array[i], kLose);
            DbManagerSetRemoteness(parents.array[i], remoteness + 1);
            int this_tier_index = child_tiers.size - 1;
            bool success = FrontierAdd(&lose_frontiers[tid], parents.array[i],
                                       remoteness + 1, this_tier_index);
            if (!success) {  // OOM.
                PositionArrayDestroy(&parents);
                return false;
            }
        }
    }
    PositionArrayDestroy(&parents);
    return true;
}

static bool ProcessTiePosition(int remoteness, TierPosition tier_position) {
    return ProcessLoseOrTiePosition(remoteness, tier_position, false);
}

static void Step5MarkDrawPositions(void) {
    PRAGMA_OMP_PARALLEL_FOR
    for (Position position = 0; position < this_tier_size; ++position) {
        if (num_undecided_children[position] > 0) {
            // A position is drawing if it still has undecided children.
            DbManagerSetValue(position, kDraw);
            continue;
        }
        assert(num_undecided_children[position] == 0);
    }
    free(num_undecided_children);
    num_undecided_children = NULL;
}

static void Step6SaveValues(void) {
    if (DbManagerFlushSolvingTier(NULL) != 0) {
        fprintf(stderr,
                "Step6SaveValues: an error has occurred while flushing of the "
                "current tier. The database file for tier %" PRITier
                " may be corrupt.\n",
                this_tier);
    }
    if (DbManagerFreeSolvingTier() != 0) {
        fprintf(stderr,
                "Step6SaveValues: an error has occurred while freeing of the "
                "current tier's in-memory database. Tier: %" PRITier "\n",
                this_tier);
    }
}

static bool CompareDb(void) {
    DbProbe probe, ref_probe;
    if (DbManagerProbeInit(&probe)) return false;
    if (DbManagerRefProbeInit(&ref_probe)) {
        DbManagerProbeDestroy(&probe);
        return false;
    }

    bool success = true;
    for (Position p = 0; p < this_tier_size; ++p) {
        TierPosition tp = {.tier = this_tier, .position = p};
        Value ref_value = DbManagerRefProbeValue(&ref_probe, tp);
        if (ref_value == kUndecided) continue;

        Value actual_value = DbManagerProbeValue(&probe, tp);
        if (actual_value != ref_value) {
            printf("CompareDb: inconsistent value at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }

        int actual_remoteness = DbManagerProbeRemoteness(&probe, tp);
        int ref_remoteness = DbManagerRefProbeRemoteness(&ref_probe, tp);
        if (actual_remoteness != ref_remoteness) {
            printf("CompareDb: inconsistent remoteness at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }
    }

_bailout:
    DbManagerProbeDestroy(&probe);
    DbManagerRefProbeDestroy(&ref_probe);
    if (success)
        printf("CompareDb: tier %" PRITier " check passed\n", this_tier);

    return success;
}

static void Step7Cleanup(void) {
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    TierArrayDestroy(&child_tiers);
    DbManagerFreeSolvingTier();
    DestroyFrontiers();
    free(num_undecided_children);
    num_undecided_children = NULL;
    if (use_reverse_graph) {
        ReverseGraphDestroy(&reverse_graph);
        // Unset the local function pointer.
        current_api.GetCanonicalParentPositions = NULL;
    }
    num_threads = 0;
}

static void DestroyFrontiers(void) {
    for (int i = 0; i < num_threads; ++i) {
        FrontierDestroy(&win_frontiers[i]);
        FrontierDestroy(&lose_frontiers[i]);
        FrontierDestroy(&tie_frontiers[i]);
    }
}

static bool TestShouldSkip(Tier tier, Position position) {
    TierPosition tier_position = {.tier = tier, .position = position};
    if (!current_api.IsLegalPosition(tier_position)) return true;
    if (current_api.Primitive(tier_position) != kUndecided) return true;

    return false;
}

static int TestChildPositions(Tier tier, Position position) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPositionArray children = current_api.GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < children.size; ++i) {
        TierPosition child = children.array[i];
        if (child.position < 0) {
            error = kTierSolverTestIllegalChildError;
            break;
        } else if (child.position >= current_api.GetTierSize(child.tier)) {
            error = kTierSolverTestIllegalChildError;
            break;
        } else if (!current_api.IsLegalPosition(child)) {
            error = kTierSolverTestIllegalChildError;
            break;
        }
    }

    return error;
}

static int TestChildToParentMatching(Tier tier, Position position) {
    TierPosition parent = {.tier = tier, .position = position};
    TierPosition canonical_parent = parent;
    canonical_parent.position =
        current_api.GetCanonicalPosition(canonical_parent);
    TierPositionArray children = current_api.GetCanonicalChildPositions(parent);
    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < children.size; ++i) {
        // Check if all child positions have parent as one of their parents.
        TierPosition child = children.array[i];
        PositionArray parents =
            current_api.GetCanonicalParentPositions(child, tier);
        if (!PositionArrayContains(&parents, canonical_parent.position)) {
            error = kTierSolverTestChildParentMismatchError;
        }

        PositionArrayDestroy(&parents);
        if (error != kTierSolverTestNoError) break;
    }
    TierPositionArrayDestroy(&children);

    return error;
}

static int TestParentToChildMatching(Tier tier, Position position,
                                     const TierArray *parent_tiers) {
    TierPosition child = {.tier = tier, .position = position};
    TierPosition canonical_child = child;
    canonical_child.position =
        current_api.GetCanonicalPosition(canonical_child);

    int error = kTierSolverTestNoError;
    for (int64_t i = 0; i < parent_tiers->size; ++i) {
        Tier parent_tier = parent_tiers->array[i];
        PositionArray parents = current_api.GetCanonicalParentPositions(
            canonical_child, parent_tier);
        for (int64_t j = 0; j < parents.size; ++j) {
            // Skip illegal and primitive parent positions as they are also
            // skipped in solving.
            TierPosition parent = {.tier = parent_tier,
                                   .position = parents.array[j]};
            if (!current_api.IsLegalPosition(parent)) continue;
            if (current_api.Primitive(parent) != kUndecided) continue;

            // Check if all parent positions have child as one of their
            // children.
            TierPositionArray children =
                current_api.GetCanonicalChildPositions(parent);
            if (!TierPositionArrayContains(&children, canonical_child)) {
                error = kTierSolverTestParentChildMismatchError;
            }

            TierPositionArrayDestroy(&children);
            if (error != kTierSolverTestNoError) break;
        }

        PositionArrayDestroy(&parents);
        if (error != kTierSolverTestNoError) break;
    }

    return error;
}

static int TestPrintError(Tier tier, Position position) {
    return printf("\nTierWorkerTest: error detected at position %" PRIPos
                  " of tier %" PRITier "\n",
                  position, tier);
}

// ===========================================================================================

static int largest_win_lose_remoteness;
static int largest_tie_remoteness;

static bool VIStep0Initialize(Tier tier) {
    this_tier = tier;
    child_tiers = current_api.GetChildTiers(this_tier);
    if (child_tiers.size == kIllegalSize) return false;

    this_tier_size = current_api.GetTierSize(tier);
    largest_win_lose_remoteness = 0;
    largest_tie_remoteness = 0;
    return true;
}

static bool VIStep1LoadChildren(void) {
    bool success = true;
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        Tier child_tier = child_tiers.array[i];
        int64_t size = current_api.GetTierSize(child_tier);
        int error = DbManagerLoadTier(child_tier, size);
        if (error != kNoError) {
            success = false;
            break;
        }

        // Scan for largest remotenesses
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(1024)
        for (Position pos = 0; pos < size; ++pos) {
            Value val = DbManagerGetValueFromLoaded(child_tier, pos);
            switch (val) {
                case kWin:
                case kLose: {
                    int r = DbManagerGetRemotenessFromLoaded(child_tier, pos);
                    if (r > largest_win_lose_remoteness) {
                        largest_win_lose_remoteness = r;
                    }
                    break;
                }

                case kTie: {
                    int r = DbManagerGetRemotenessFromLoaded(child_tier, pos);
                    if (r > largest_tie_remoteness) {
                        largest_tie_remoteness = r;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

    return success;
}

static bool VIStep3SetupSolvingTier(void) {
    int error = DbManagerCreateSolvingTier(this_tier, this_tier_size);
    if (error != kNoError) return false;

    return true;
}

static void VIStep4ScanTier(void) {
    PRAGMA_OMP_PARALLEL {
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            TierPosition tier_position = {.tier = this_tier, .position = pos};
            if (!current_api.IsLegalPosition(tier_position) ||
                !IsCanonicalPosition(pos)) {
                // Temporarily mark illegal and non-canonical positions as
                // drawing. These values will be changed to undecided later.
                DbManagerSetValue(pos, kDraw);
                continue;
            }

            Value value = current_api.Primitive(tier_position);
            if (value != kUndecided) {  // If tier_position is primitive...
                // Set its value immediately.
                DbManagerSetValue(pos, value);
                DbManagerSetRemoteness(pos, 0);
            }  // Otherwise, do nothing.
        }
    }
}

static bool IterateWinLoseProcessPosition(int iteration, Position pos,
                                          bool *updated) {
    *updated = false;
    bool all_children_winning = true;
    int largest_win = -1;
    TierPosition tier_position = {.tier = this_tier, .position = pos};
    TierPositionArray child_positions =
        current_api.GetCanonicalChildPositions(tier_position);
    if (child_positions.size == kIllegalSize) return false;

    for (int64_t i = 0; i < child_positions.size; ++i) {
        TierPosition child_tier_position = child_positions.array[i];
        Value child_value;
        int child_remoteness;
        if (child_tier_position.tier == this_tier) {
            child_value = DbManagerGetValue(child_tier_position.position);
            child_remoteness =
                DbManagerGetRemoteness(child_tier_position.position);
        } else {
            child_value = DbManagerGetValueFromLoaded(
                child_tier_position.tier, child_tier_position.position);
            child_remoteness = DbManagerGetRemotenessFromLoaded(
                child_tier_position.tier, child_tier_position.position);
        }
        switch (child_value) {
            case kUndecided:
            case kTie:
                all_children_winning = false;
                break;
            case kLose:
                all_children_winning = false;
                if (child_remoteness == iteration - 1) {
                    DbManagerSetValue(pos, kWin);
                    DbManagerSetRemoteness(pos, iteration);
                    *updated = true;
                    TierPositionArrayDestroy(&child_positions);
                    return true;
                }
                break;
            case kWin:
                if (child_remoteness > largest_win) {
                    largest_win = child_remoteness;
                }
                break;
            default:  // Do nothing about tying positions yet.
                break;
        }
    }

    if (all_children_winning && largest_win + 1 == iteration) {
        DbManagerSetValue(pos, kLose);
        DbManagerSetRemoteness(pos, iteration);
        *updated = true;
    }

    TierPositionArrayDestroy(&child_positions);
    return true;
}

static bool VIStep4_0IterateWinLose(void) {
    bool updated = false;
    bool failed = false;

    for (int i = 1; updated || i <= largest_win_lose_remoteness + 1; ++i) {
        updated = false;
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(1024)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            bool pos_updated;
            if (DbManagerGetValue(pos) != kUndecided) continue;
            bool success = IterateWinLoseProcessPosition(i, pos, &pos_updated);
            if (!success) failed = true;
            if (pos_updated) updated = true;
        }

        if (failed) return false;
    }

    return true;
}

static bool IterateTieProcessPosition(int iteration, Position pos,
                                      bool *updated) {
    *updated = false;
    TierPosition tier_position = {.tier = this_tier, .position = pos};
    TierPositionArray child_positions =
        current_api.GetCanonicalChildPositions(tier_position);
    if (child_positions.size == kIllegalSize) return false;

    for (int64_t i = 0; i < child_positions.size; ++i) {
        TierPosition child_tier_position = child_positions.array[i];
        Value child_value;
        int child_remoteness;
        if (child_tier_position.tier == this_tier) {
            child_value = DbManagerGetValue(child_tier_position.position);
            child_remoteness =
                DbManagerGetRemoteness(child_tier_position.position);
        } else {
            child_value = DbManagerGetValueFromLoaded(
                child_tier_position.tier, child_tier_position.position);
            child_remoteness = DbManagerGetRemotenessFromLoaded(
                child_tier_position.tier, child_tier_position.position);
        }
        if (child_value == kTie && child_remoteness == iteration - 1) {
            DbManagerSetValue(pos, kTie);
            DbManagerSetRemoteness(pos, iteration);
            *updated = true;
            break;
        }
    }

    TierPositionArrayDestroy(&child_positions);
    return true;
}

static bool VIStep4_1IterateTie(void) {
    bool updated = false;
    bool failed = false;

    for (int i = 1; updated || i <= largest_tie_remoteness + 1; ++i) {
        updated = false;
        PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(1024)
        for (Position pos = 0; pos < this_tier_size; ++pos) {
            bool pos_updated;
            if (DbManagerGetValue(pos) != kUndecided) continue;
            bool success = IterateTieProcessPosition(i, pos, &pos_updated);
            if (!success) failed = true;
            if (pos_updated) updated = true;
        }

        if (failed) return false;
    }

    return true;
}

static bool VIStep4Iterate(void) {
    bool success = VIStep4_0IterateWinLose();
    if (!success) return false;

    success = VIStep4_1IterateTie();
    if (!success) return false;

    // The child tiers are no longer needed.
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        DbManagerUnloadTier(child_tiers.array[i]);
    }

    return true;
}

static void VIStep5MarkDrawPositions(void) {
    PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(1024)
    for (Position pos = 0; pos < this_tier_size; ++pos) {
        Value val = DbManagerGetValue(pos);
        if (val == kUndecided)
            DbManagerSetValue(pos, kDraw);
        else if (val == kDraw)
            DbManagerSetValue(pos, kUndecided);
    }
}

static void VIStep6FlushDb(void) {
    if (DbManagerFlushSolvingTier(NULL) != 0) {
        fprintf(stderr,
                "VIStep6FlushDb: an error has occurred while flushing of the "
                "current tier. The database file for tier %" PRITier
                " may be corrupt.\n",
                this_tier);
    }
    if (DbManagerFreeSolvingTier() != 0) {
        fprintf(stderr,
                "VIStep6FlushDb: an error has occurred while freeing of the "
                "current tier's in-memory database. Tier: %" PRITier "\n",
                this_tier);
    }
}

static void VIStep7Cleanup(void) {
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        Tier child_tier = child_tiers.array[i];
        if (DbManagerIsTierLoaded(child_tier)) DbManagerUnloadTier(child_tier);
    }
    TierArrayDestroy(&child_tiers);
    DbManagerFreeSolvingTier();
}

int TierWorkerSolveValueIteration(Tier tier, bool force, bool compare,
                                  bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        ret = kNoError;  // Success.
        goto _bailout;
    }

    /* Value Iteration main algorithm. */
    if (!VIStep0Initialize(tier)) goto _bailout;
    if (!VIStep1LoadChildren()) goto _bailout;
    if (!VIStep3SetupSolvingTier()) goto _bailout;
    VIStep4ScanTier();
    if (!VIStep4Iterate()) goto _bailout;
    VIStep5MarkDrawPositions();
    VIStep6FlushDb();
    if (compare && !CompareDb()) goto _bailout;
    if (solved != NULL) *solved = true;
    ret = kNoError;  // Success.

_bailout:
    VIStep7Cleanup();
    return ret;
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL
#undef PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC
#undef PRAGMA_OMP_PARALLEL_FOR
#undef PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC
