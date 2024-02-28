#include "games/quixo/quixo.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

static const int kSideLength = 5;
static const int kBoardSize = kSideLength * kSideLength;

static int QuixoInit(void *aux);
static int InitGenericHash(void);
static bool IsValidPieceConfig(int blanks, int xs, int os);
static Tier HashTier(int blanks, int xs, int os);

static int QuixoFinalize(void);

const Game kQuixo = {
    .name = "quixo",
    .formal_name = "Quixo",
    .solver = &kTierSolver,
    .solver_api = (const void *)NULL,           // TODO
    .gameplay_api = (const GameplayApi *)NULL,  // TODO
    .uwapi = NULL,                              // TODO

    .Init = &QuixoInit,
    .Finalize = &QuixoFinalize,

    .GetCurrentVariant = NULL,
    .SetVariantOption = NULL,
};

static int QuixoInit(void *aux) {
    (void)aux;  // Unused.
    return InitGenericHash();
}

static int InitGenericHash(void) {
    GenericHashReinitialize();
    int player = 0;  // Default to two players.
    int pieces_init_array[10] = {'-', 25, 25, 'X', 0, 0, 'O', 0, 0, -1};

    bool success;
    for (int blanks = 0; blanks <= 25; ++blanks) {
        for (int xs = 0; xs <= 25; ++xs) {
            for (int os = 0; os <= 25; ++os) {
                if (!IsValidPieceConfig(blanks, xs, os)) continue;
                Tier tier = HashTier(blanks, xs, os);
                pieces_init_array[1] = pieces_init_array[2] = blanks;
                pieces_init_array[4] = pieces_init_array[5] = xs;
                pieces_init_array[7] = pieces_init_array[8] = os;

                success = GenericHashAddContext(player, kBoardSize,
                                                pieces_init_array, NULL, tier);
                if (!success) {
                    fprintf(
                        stderr,
                        "InitGenericHash: failed to initialize generic hash "
                        "context for tier %" PRITier ". Aborting...\n",
                        tier);
                    GenericHashReinitialize();
                    return kRuntimeError;
                }
            }
        }
    }

    return kNoError;
}

static bool IsValidPieceConfig(int blanks, int xs, int os) {
    if (blanks < 0 || xs < 0 || os < 0) return false;
    if (blanks + xs + os != kBoardSize) return false;
    if (blanks <= 23 && (xs == 0 || os == 0)) return false;
    if (blanks == 24 && (xs != 1 || os != 0)) return false;
    if (blanks == 25 && (xs != 0 || os != 0)) return false;

    return true;
}

static Tier HashTier(int blanks, int xs, int os) {
    return blanks * kBoardSize * kBoardSize + xs * kBoardSize + os;
}

static int QuixoFinalize(void) {
    GenericHashReinitialize();
    return kNoError;
}
