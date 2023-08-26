#include "games/mtttier/mtttier.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // atoi

#include "core/gamesman_types.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"

// Game, Solver, and Gameplay API Functions

static int MtttierInit(void *aux);
static int MtttierFinalize(void);

static const GameVariant *MtttierGetCurrentVariant(void);
static int MtttierSetVariantOption(int option, int selection);

static Tier MtttierGetInitialTier(void);
static Position MtttierGetInitialPosition(void);

static int64_t MtttierGetTierSize(Tier tier);
static MoveArray MtttierGenerateMoves(TierPosition tier_position);
static Value MtttierPrimitive(TierPosition tier_position);
static TierPosition MtttierDoMove(TierPosition tier_position, Move move);
static bool MtttierIsLegalPosition(TierPosition tier_position);
static Position MtttierGetCanonicalPosition(TierPosition tier_position);
static PositionArray MtttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier);
static TierArray MtttierGetChildTiers(Tier tier);
static TierArray MtttierGetParentTiers(Tier tier);

static int MtttTierPositionToString(TierPosition tier_position,
                                       char *buffer);
static int MtttierMoveToString(Move move, char *buffer);
static bool MtttierIsValidMoveString(const char *move_string);
static Move MtttierStringToMove(const char *move_string);

// Solver API Setup
static const TierSolverApi kSolverApi = {
    .GetInitialTier = &MtttierGetInitialTier,
    .GetInitialPosition = &MtttierGetInitialPosition,

    .GetTierSize = &MtttierGetTierSize,
    .GenerateMoves = &MtttierGenerateMoves,
    .Primitive = &MtttierPrimitive,
    .DoMove = &MtttierDoMove,
    .IsLegalPosition = &MtttierIsLegalPosition,
    .GetCanonicalPosition = &MtttierGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MtttierGetCanonicalParentPositions,
    .GetPositionInSymmetricTier = NULL,
    .GetChildTiers = &MtttierGetChildTiers,
    .GetParentTiers = &MtttierGetParentTiers,
    .GetCanonicalTier = NULL,
};

// Gameplay API Setup
static const GameplayApi kGameplayApi = {
    .GetInitialTier = &MtttierGetInitialTier,
    .GetInitialPosition = &MtttierGetInitialPosition,

    .position_string_length_max = 120,
    .TierPositionToString = &MtttTierPositionToString,

    .move_string_length_max = 1,
    .MoveToString = &MtttierMoveToString,

    .IsValidMoveString = &MtttierIsValidMoveString,
    .StringToMove = &MtttierStringToMove,

    .TierGenerateMoves = &MtttierGenerateMoves,
    .TierDoMove = &MtttierDoMove,
    .TierPrimitive = &MtttierPrimitive,

    .TierGetCanonicalPosition = &MtttierGetCanonicalPosition,

    .GetCanonicalTier = NULL,
    .GetPositionInSymmetricTier = NULL,
};

const Game kMtttier = {
    .name = "mtttier",
    .formal_name = "Tic-Tac-Tier",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kGameplayApi,

    .Init = &MtttierInit,
    .Finalize = &MtttierFinalize,

    .GetCurrentVariant = &MtttierGetCurrentVariant,
    .SetVariantOption = &MtttierSetVariantOption,
};

// Helper Types and Global Variables

static const int kNumRowsToCheck = 8;
static const int kRowsToCheck[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
    {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6},
};
static const int kNumSymmetries = 8;
static const int kSymmetryMatrix[8][9] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8}, {2, 5, 8, 1, 4, 7, 0, 3, 6},
    {8, 7, 6, 5, 4, 3, 2, 1, 0}, {6, 3, 0, 7, 4, 1, 8, 5, 2},
    {2, 1, 0, 5, 4, 3, 8, 7, 6}, {0, 3, 6, 1, 4, 7, 2, 5, 8},
    {6, 7, 8, 3, 4, 5, 0, 1, 2}, {8, 5, 2, 7, 4, 1, 6, 3, 0},
};

// Helper Functions

static bool InitGenericHash(void);
static char ThreeInARow(const char *board, const int *indices);
static bool AllFilledIn(const char *board);
static void CountPieces(const char *board, int *xcount, int *ocount);
static char WhoseTurn(const char *board);
static Position DoSymmetry(TierPosition tier_position, int symmetry);
static char ConvertBlankToken(char piece);

static int MtttierInit(void *aux) {
    (void)aux;  // Unused.
    return !InitGenericHash();
}

static int MtttierFinalize(void) { return 0; }

static const GameVariant *MtttierGetCurrentVariant(void) {
    return NULL;  // Not implemented.
}

static int MtttierSetVariantOption(int option, int selection) {
    (void)option;
    (void)selection;
    return 0;  // Not implemented.
}

static Tier MtttierGetInitialTier(void) { return 0; }

// Assumes Generic Hash has been initialized.
static Position MtttierGetInitialPosition(void) {
    char board[9];
    for (int i = 0; i < 9; ++i) {
        board[i] = '-';
    }
    return GenericHashHashLabel(0, board, 1);
}

static int64_t MtttierGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray MtttierGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    for (Move i = 0; i < 9; ++i) {
        if (board[i] == '-') {
            MoveArrayAppend(&moves, i);
        }
    }
    return moves;
}

static Value MtttierPrimitive(TierPosition tier_position) {
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    for (int i = 0; i < kNumRowsToCheck; ++i) {
        if (ThreeInARow(board, kRowsToCheck[i]) > 0) return kLose;
    }
    if (AllFilledIn(board)) return kTie;
    return kUndecided;
}

static TierPosition MtttierDoMove(TierPosition tier_position, Move move) {
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    char turn = WhoseTurn(board);
    board[move] = turn;
    TierPosition ret;
    ret.tier = tier_position.tier + 1;
    ret.position = GenericHashHashLabel(tier_position.tier, board, 1);
    return ret;
}

static bool MtttierIsLegalPosition(TierPosition tier_position) {
    // A position is legal if and only if:
    // 1. xcount == ocount or xcount = ocount + 1 if no one is winning and
    // 2. xcount == ocount if O is winning and
    // 3. xcount == ocount + 1 if X is winning and
    // 4. only one player can be winning
    char board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount != ocount && xcount != ocount + 1) return false;

    bool xwin = false, owin = false;
    for (int i = 0; i < kNumRowsToCheck; ++i) {
        char row_value = ThreeInARow(board, kRowsToCheck[i]);
        xwin |= (row_value == 'X');
        owin |= (row_value == 'O');
    }
    if (xwin && owin) return false;
    if (xwin && xcount != ocount + 1) return false;
    if (owin && xcount != ocount) return false;
    return true;
}

static Position MtttierGetCanonicalPosition(TierPosition tier_position) {
    Position canonical_position = tier_position.position;
    Position new_position;

    for (int i = 0; i < kNumSymmetries; ++i) {
        new_position = DoSymmetry(tier_position, i);
        if (new_position < canonical_position) {
            // By GAMESMAN convention, the canonical position is the one with
            // the smallest hash value.
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

static PositionArray MtttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    //
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    PositionArray parents;
    PositionArrayInit(&parents);
    if (parent_tier != tier - 1) return parents;

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);

    char prev_turn = WhoseTurn(board) == 'X' ? 'O' : 'X';
    for (int i = 0; i < 9; ++i) {
        if (board[i] == prev_turn) {
            // Take piece off the board.
            board[i] = '-';
            TierPosition parent = {
                .tier = tier - 1,
                .position = GenericHashHashLabel(tier - 1, board, 1)};
            // Add piece back to the board.
            board[i] = prev_turn;
            if (!MtttierIsLegalPosition(parent)) {
                continue;  // Illegal.
            }
            parent.position = MtttierGetCanonicalPosition(parent);
            if (PositionHashSetContains(&deduplication_set, parent.position)) {
                continue;  // Already included.
            }
            PositionHashSetAdd(&deduplication_set, parent.position);
            PositionArrayAppend(&parents, parent.position);
        }
    }
    PositionHashSetDestroy(&deduplication_set);

    return parents;
}

static TierArray MtttierGetChildTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    if (tier < 9) TierArrayAppend(&children, tier + 1);
    return children;
}

static TierArray MtttierGetParentTiers(Tier tier) {
    TierArray parents;
    TierArrayInit(&parents);
    if (tier > 0) TierArrayAppend(&parents, tier - 1);
    return parents;
}

static int MtttTierPositionToString(TierPosition tier_position,
                                       char *buffer) {
    char board[9] = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return 1;

    for (int i = 0; i < 9; ++i) {
        board[i] = ConvertBlankToken(board[i]);
    }

    static const char *format =
        "         ( 1 2 3 )           : %c %c %c\n"
        "LEGEND:  ( 4 5 6 )  TOTAL:   : %c %c %c\n"
        "         ( 7 8 9 )           : %c %c %c";
    int actual_length =
        snprintf(buffer, kGameplayApi.position_string_length_max + 1, format,
                 board[0], board[1], board[2], board[3], board[4], board[5],
                 board[6], board[7], board[8]);
    if (actual_length >= kGameplayApi.position_string_length_max + 1) {
        fprintf(
            stderr,
            "MtttierTierPositionToString: (BUG) not enough space was allocated "
            "to buffer. Please increase position_string_length_max.\n");
        return 1;
    }
    return 0;
}

static int MtttierMoveToString(Move move, char *buffer) {
    int actual_length = snprintf(
        buffer, kGameplayApi.move_string_length_max + 1, "%" PRId64, move + 1);
    if (actual_length >= kGameplayApi.move_string_length_max + 1) {
        fprintf(stderr,
                "MtttierMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }
    return 0;
}

static bool MtttierIsValidMoveString(const char *move_string) {
    // Only "1" - "9" are valid move strings.
    if (move_string[0] < '1') return false;
    if (move_string[0] > '9') return false;
    if (move_string[1] != '\0') return false;

    return true;
}

static Move MtttierStringToMove(const char *move_string) {
    assert(MtttierIsValidMoveString(move_string));
    return (Move)atoi(move_string) - 1;
}

// Helper functions implementation

static bool InitGenericHash(void) {
    GenericHashReinitialize();
    int player = 1;  // No turn bit needed as we can infer the turn from board.
    int board_size = 9;
    int pieces_init_array[10] = {'-', 9, 9, 'O', 0, 0, 'X', 0, 0, -1};
    for (Tier tier = 0; tier <= 9; ++tier) {
        // Adjust piece_init_array
        pieces_init_array[1] = pieces_init_array[2] = 9 - tier;
        pieces_init_array[4] = pieces_init_array[5] = tier / 2;
        pieces_init_array[7] = pieces_init_array[8] = (tier + 1) / 2;
        bool success = GenericHashAddContext(player, board_size,
                                             pieces_init_array, NULL, tier);
        if (!success) {
            fprintf(stderr,
                    "MtttierInit: failed to initialize generic hash context "
                    "for tier %" PRId64 ". Aborting...\n",
                    tier);
            GenericHashReinitialize();
            return false;
        }
    }
    return true;
}

static char ThreeInARow(const char *board, const int *indices) {
    if (board[indices[0]] != board[indices[1]]) return 0;
    if (board[indices[1]] != board[indices[2]]) return 0;
    if (board[indices[0]] == '-') return 0;
    return board[indices[0]];
}

static bool AllFilledIn(const char *board) {
    for (int i = 0; i < 9; ++i) {
        if (board[i] == '-') return false;
    }
    return true;
}

static void CountPieces(const char *board, int *xcount, int *ocount) {
    *xcount = *ocount = 0;
    for (int i = 0; i < 9; ++i) {
        *xcount += (board[i] == 'X');
        *ocount += (board[i] == 'O');
    }
}

static char WhoseTurn(const char *board) {
    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount == ocount) return 'X';  // In our TicTacToe, x always goes first.
    return 'O';
}

static Position DoSymmetry(TierPosition tier_position, int symmetry) {
    char board[9] = {0}, symmetry_board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    // Copy from the symmetry matrix.
    for (int i = 0; i < 9; ++i) {
        symmetry_board[i] = board[kSymmetryMatrix[symmetry][i]];
    }

    return GenericHashHashLabel(tier_position.tier, symmetry_board, 1);
}

static char ConvertBlankToken(char piece) {
    if (piece == '-') return ' ';
    return piece;
}
