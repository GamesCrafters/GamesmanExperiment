/**
 * @file mkaooa.c
 * @author 
 *  Xiang Zheng
 *  Sriya Kantipudi
 *  Maria Rufova
 *  Benji Xu
 *  Robert Shi
 * Supervised by Dan Garcia
 * @brief Kaooa implementation
 * @version 1.0.2
 * @date 2024-10-18
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

#include "games/mkaooa/mkaooa.h"

#include <assert.h>    // assert
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // atoi

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, and Gameplay API Functions

#define boardSize 10

// MB TODO: Can we just use All Quenes Chess's unhash move and hash move? 
#define MOVE_ENCODE(from, to) ((from << 5) | to) 

// NOTE: Player 1 is always (C)row, Plaer 2 is always (V)ulture
// Player 1 Crow goes first!
#define C 'C'
#define V 'V'
#define BLANK '-'

static int MkaooaInit(void *aux);
static int MkaooaFinalize(void);

static const GameVariant *MkaooaGetCurrentVariant(void);
static int MkaooaSetVariantOption(int option, int selection);

static int64_t MkaooaGetNumPositions(void);
static Position MkaooaGetInitialPosition(void);

static MoveArray MkaooaGenerateMoves(Position position);
static Value MkaooaPrimitive(Position position);
static Position MkaooaDoMove(Position position, Move move);
static bool MkaooaIsLegalPosition(Position position);
static Position MkaooaGetCanonicalPosition(Position position);
static PositionArray MkaooaGetCanonicalParentPositions(
    Position position);

static int MkaooaPositionToString(Position position, char *buffer);
static int MkaooaMoveToString(Move move, char *buffer);
static bool MkaooaIsValidMoveString(ReadOnlyString move_string);
static Move MkaooaStringToMove(ReadOnlyString move_string);

// Solver API Setup
static const RegularSolverApi kSolverApi = {
    .GetNumPositions = &MkaooaGetNumPositions,
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .GenerateMoves = &MkaooaGenerateMoves,
    .Primitive = &MkaooaPrimitive,
    .DoMove = &MkaooaDoMove,
    .IsLegalPosition = &MkaooaIsLegalPosition,
    .GetCanonicalPosition = &MkaooaGetCanonicalPosition, // MB TODO: Can change this to null? 
    .GetNumberOfCanonicalChildPositions = NULL,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MkaooaGetCanonicalParentPositions, // MB TODO: Can change this to null? 
};

// Gameplay API Setup

static const GameplayApiCommon kGamePlayApiCommon = {
    .GetInitialPosition = &MkaooaGetInitialPosition,
    .position_string_length_max = 120,

    .move_string_length_max = 3,
    .MoveToString = &MkaooaMoveToString,
    .IsValidMoveString = &MkaooaIsValidMoveString,
    .StringToMove = &MkaooaStringToMove,
};

static const GameplayApiRegular kGameplayApiRegular = {
    .PositionToString = &MkaooaPositionToString,
    .GenerateMoves = &MkaooaGenerateMoves,
    .DoMove = &MkaooaDoMove,
    .Primitive = &MkaooaPrimitive,
};

static const GameplayApi kGameplayApi = {
    .common = &kGamePlayApiCommon,
    .regular = &kGameplayApiRegular,
};

const Game kMkaooa = {
    .name = "mkaooa",
    .formal_name = "Kaooa",
    .solver = &kRegularSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kGameplayApi,

    .Init = &MkaooaInit,
    .Finalize = &MkaooaFinalize,

    .GetCurrentVariant = &MkaooaGetCurrentVariant,
    .SetVariantOption = &MkaooaSetVariantOption,
};

// Helper Functions

//this is good 
static void UnhashMove(Move move, int *from, int *to);


static int MkaooaInit(void *aux) {
    (void)aux;  // Unused.

    GenericHashReinitialize();
    int pieces_init_array[10] = {BLANK, 10, 10, C, 6, 6, V, 1, 1, -1}; // MB TODO: Figure out what this array should be
    bool success =
        GenericHashAddContext(0, boardSize, pieces_init_array, NULL, 0);
    if (!success) {
        fprintf(stderr,
                "Mkaooa: failed to initialize generic hash context. "
                "Aborting...\n");
        GenericHashReinitialize();
        return kRuntimeError;
    }
    return kNoError;
}

static int MkaooaFinalize(void) { return kNoError; }

static const GameVariant *MkaooaGetCurrentVariant(void) {
    return NULL;  // Later MB TODO
}

//start off with 6 crows 
static int MkaooaSetVariantOption(int option, int selection) {
    (void)option;
    (void)selection;
    return 0;  // Later MB TODO
}

// TODO: Hash initial board configuration
static Position MkaooaGetInitialPosition(void) {
    return GenericHashHash("--------------------", 1); // HINT: second parameter should be left as 1
                            //WBWBW-----B---W-----BWBWB", 1); // HINT: second parameter should be left as 1
    //change this to our gameboard, so just put all dashes here 
}


static int64_t MkaooaGetNumPositions(void) {
    return GenericHashNumPositions();
}

// TODO
// Given a board position, generate all the moves available depending on whose turn it is.
// Position is a 64 bit integer. For us, we will use the last 11 digut of this number. 
// The 11th digit would represent the # Crows dropped (0 - 6), and rest 10 digit would each correspond to a spot on the board (0 or 1 or 2)
static MoveArray MkaooaGenerateMoves(Position position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[boardSize];
    GenericHashUnhash(position, board);

    char turn = GenericHashGetTurn(position) == 1 ? C : V;

    // NOTE: The following is an example of how possible moves were calculated for a piece in All Queens Chess. 
    // You will not need to write as much because pieces in Kaooa generally have much less moves available to them.
    // You do not need to change the code above
    for (int i = 0; i < boardSize; i++) {
        if ((turn == C && board[i] == C) || (turn == V && board[i] == V)) {
            int originRow = i / sideLength;
            int originCol = i % sideLength;
            int origin = i;

            // Left
            for (int col = originCol - 1; col >= 0; col--) {
                if (board[originRow * sideLength + col] == BLANK) {
                    int targetRow = originRow;
                    int targetCol = col;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Right
            for (int col = originCol + 1; col < sideLength; col++) {
                if (board[originRow * sideLength + col] == BLANK) {
                    int targetRow = originRow;
                    int targetCol = col;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Up
            for (int row = originRow - 1; row >= 0; row--) {
                if (board[row * sideLength + originCol] == BLANK) {
                    int targetRow = row;
                    int targetCol = originCol;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Down
            for (int row = originRow + 1; row < sideLength; row++) {
                if (board[row * sideLength + originCol] == BLANK) {
                    int targetRow = row;
                    int targetCol = originCol;

                    int target = targetRow * sideLength + targetCol;

                    MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));
                } else {
                    break;
                }
            }

            // Left-Up
            if (originRow > 0 && originCol > 0) {
                int row = originRow - 1;
                int col = originCol - 1;

                while (row >= 0 && col >= 0) {
                    if (board[row * sideLength + col] == BLANK) {
                        int target = row * sideLength + col;
                        MoveArrayAppend(&moves, MOVE_ENCODE(origin, target));

                        row--;
                        col--;
                    } else {
                        break;
                    }
                }
            }
        }
    }

    return moves;
}

// TODO: Checks if a given board position is Primitive.
// A Primitive position is one that signals the end of the game.
// Hint: At what point do we know that the game is lost?
// The game ends when either the vulture captures 3 crows OR when the vulture is trapped
// For our game, we would only return kLose or kUndecided (reasons explained during meeting)
static Value MkaooaPrimitive(Position position) {
    char board[boardSize];
    GenericHashUnhash(position, board);

    // Code below are examples from All Queuens Chess. You can keep the 2 lines above unchanged. 

    char piece;

    // Vertical
    int i = 0;
    for (i = 10; i < 15; i++) {
        piece = board[i];
        if (piece != BLANK) {
            if (board[i - 5] == piece && board[i + 5] == piece) {
                if (board[i - 10] == piece || board[i + 10] == piece) {
                    return kLose;
                }
            }
        }
    }

    // Horizontal
    for (i = 2; i < 25; i += 5) {
        piece = board[i];
        if (piece != BLANK) {
            if (board[i - 1] == piece && board[i + 1] == piece) {
                if (board[i - 2] == piece || board[i + 2] == piece) {
                    return kLose;
                }
            }
        }
    }

    piece = board[12];
    if (piece != BLANK) {
        // Antidiagonal
        if (board[6] == piece && board[18] == piece) {
            if (board[0] == piece || board[24] == piece) {
                return kLose;
            }
        }
        // Maindiagonal
        if (board[8] == piece && board[16] == piece) {
            if (board[4] == piece || board[20] == piece) {
                return kLose;
            }
        }
    }

    piece = board[1];
    if (piece != BLANK && board[7] == piece && board[13] == piece &&
        board[19] == piece) {
        return kLose;
    }

    return kUndecided;
}

// TODO: Takes in a Position and a Move, return a new Position (hashed) which is generated from performing the MOVE to POSITION
// Refer to psuedocode in slack!
static Position MkaooaDoMove(Position position, Move move) {
    char board[boardSize];
    GenericHashUnhash(position, board);

    int from, to;
    UnhashMove(move, &from, &to);

    // The code above can be left unchanged

    board[to] = board[from];
    board[from] = BLANK;

    int oppTurn = GenericHashGetTurn(position) == 1 ? 2 : 1;
    return GenericHashHash(board, oppTurn);
}

static bool MkaooaIsLegalPosition(Position position) { // MB TODO: Do we need to implement this? 
    // Don't need to implement.
    (void)position;
    return true;
}

// MB TODO: Not considering symmetries rn, but think about if we actually need this

// static Position MkaooaGetCanonicalPosition(Position position) {
//     char board[boardSize];
//     GenericHashUnhash(position, board);

//     char pieceInSymmetry, pieceInCurrentCanonical;
//     int i, symmetryNum;
//     int turn = GenericHashGetTurn(position);

//     /* Figure out which symmetry transformation on the input board
//     leads to the smallest-ternary-number board in the input board's orbit
//     (where the transformations are just rotation/reflection. */
//     int bestSymmetryNum = 0;
//     for (symmetryNum = 1; symmetryNum < totalNumBoardSymmetries; symmetryNum++) {
//         for (i = boardSize - 1; i >= 0; i--) {
//             pieceInSymmetry = board[symmetries[symmetryNum][i]];
//             pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
//             if (pieceInSymmetry != pieceInCurrentCanonical) {
//                 if (pieceInSymmetry < pieceInCurrentCanonical) {
//                     bestSymmetryNum = symmetryNum;
//                 }
//                 break;
//             }
//         }
//     }
//     char canonBoard[boardSize];
//     for (i = 0; i < boardSize; i++) { // Transform the rest of the board.
//         canonBoard[i] = board[symmetries[bestSymmetryNum][i]];
//     }

//     // Invert the piece colors of the board
//     for (i = 0; i < boardSize; i++) {
//         if (board[i] == W) {
//             board[i] = B;
//         } else if (board[i] == B) {
//             board[i] = W;
//         }
//     }

//     /* Figure out which symmetry transformation on the swapped input board
//     leads to the smallest-ternary-number board in the input board's orbit
//     (where the transformations are just rotation/reflection. */
//     bestSymmetryNum = 0;
//     for (symmetryNum = 1; symmetryNum < totalNumBoardSymmetries; symmetryNum++) {
//         for (i = boardSize - 1; i >= 0; i--) {
//             pieceInSymmetry = board[symmetries[symmetryNum][i]];
//             pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
//             if (pieceInSymmetry != pieceInCurrentCanonical) {
//                 if (pieceInSymmetry < pieceInCurrentCanonical) {
//                     bestSymmetryNum = symmetryNum;
//                 }
//                 break;
//             }
//         }
//     }
//     char canonSwappedBoard[boardSize];
//     for (i = 0; i < boardSize; i++) { // Transform the rest of the board.
//         canonSwappedBoard[i] = board[symmetries[bestSymmetryNum][i]];
//     }

//     // Compare canonBoard and canonSwappedBoard
//     char pieceInRegular, pieceInSwapped;
//     for (i = boardSize - 1; i >= 0; i--) {
//         pieceInRegular = canonBoard[i];
//         pieceInSwapped = canonSwappedBoard[i];
//         if (pieceInRegular < pieceInSwapped) {
//             return GenericHashHash(canonBoard, turn);
//         } else if (pieceInSwapped < pieceInRegular) {
//             return GenericHashHash(canonSwappedBoard, turn == 1 ? 2 : 1);
//         }
//     }
//     return GenericHashHash(canonBoard, 1);
// }

// static PositionArray MkaooaGetCanonicalParentPositions(
//     Position position) {
//     /* The parent positions can be found by swapping the turn of
//     the position to get position P', getting the children of
//     P', canonicalizing them, then swapping the turn of each
//     of those canonical children. */

//     char board[boardSize];
//     GenericHashUnhash(position, board);
//     int t = GenericHashGetTurn(position);
//     int oppT = t == 1 ? 2 : 1;
//     Position turnSwappedPos = GenericHashHash(board, oppT);

//     PositionHashSet deduplication_set;
//     PositionHashSetInit(&deduplication_set, 0.5);

//     PositionArray canonicalParents;
//     PositionArrayInit(&canonicalParents);

//     Position child;
//     MoveArray moves = MkaooaGenerateMoves(turnSwappedPos);
//     for (int64_t i = 0; i < moves.size; i++) {
//         child = MkaooaDoMove(turnSwappedPos, moves.array[i]);
//         /* Note that at this point, it is current player's turn at `child`.
//         We check if it's primitive before we swap the turn because
//         primitive doesn't care about turn. */
//         if (MkaooaPrimitive(child) == kUndecided) {
//             GenericHashUnhash(child, board);
//             child = GenericHashHash(board, oppT);  // Now it's opponent's turn
//             child = MkaooaGetCanonicalPosition(child);
//             if (!PositionHashSetContains(&deduplication_set, child)) {
//                 PositionHashSetAdd(&deduplication_set, child);
//                 PositionArrayAppend(&canonicalParents, child);
//             }
//         }
//     }
//     PositionHashSetDestroy(&deduplication_set);
//     MoveArrayDestroy(&moves);
//     return canonicalParents;
// }


// TODO: Takes in a POSITION, fills its string representation in the BUFFER. 
// This is to display the board/position to the user when using GamesmanOne
static int MkaooaPositionToString(Position position, char *buffer) {
    char board[boardSize];
    GenericHashUnhash(position, board);
    char turn = GenericHashGetTurn(position) == 1 ? W : B;

    static ConstantReadOnlyString kFormat =
        "\n"
        "1 %c%c%c%c%c\n"
        "2 %c%c%c%c%c\n"
        "3 %c%c%c%c%c\n"
        "4 %c%c%c%c%c\n"
        "5 %c%c%c%c%c\n"
        "  abcde          TURN: %c\n";

    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.position_string_length_max + 1, kFormat,
        board[0], board[1], board[2], board[3], board[4], board[5], board[6],
        board[7], board[8], board[9], board[10], board[11], board[12],
        board[13], board[14], board[15], board[16], board[17], board[18],
        board[19], board[20], board[21], board[22], board[23], board[24], turn);

    if (actual_length >= kGamePlayApiCommon.position_string_length_max + 1) {
        fprintf(stderr,
                "MkaooaTierPositionToString: (BUG) not enough space "
                "was allocated "
                "to buffer. Please increase position_string_length_max.\n");
        return 1;
    }
    return 0;
}

// TODO: Takes in a MOVE, fills its string representation in the BUFFER. 
// This is to display the move to the user when using GamesmanOne
// The string representation of a move can be a 2-character string seperated by a white space. Eg: 
// 'X Y', where X is the source (0 - 9) and Y is the destination (0 - 9)
// When X = Y, the move signifies dropping a piece
static int MkaooaMoveToString(Move move, char *buffer) {
    int from, to;
    UnhashMove(move, &from, &to);
    int fromRow = from / sideLength;
    int fromCol = from % sideLength;
    int toRow = to / sideLength;
    int toCol = to % sideLength;

    int actual_length = snprintf(
        buffer, kGamePlayApiCommon.move_string_length_max + 1, "%d%c%d%c",
        fromRow + 1, fromCol + 'a', toRow + 1, toCol + 'a');
    if (actual_length >= kGamePlayApiCommon.move_string_length_max + 1) {
        fprintf(
            stderr,
            "MkaooaMoveToString: (BUG) not enough space was allocated "
            "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }
    return 0;
}

// TODO
// Checks if string representing a move is valid
// This is NOT the same as checking if a move is a valid move. Here you are only supposed to check if a string is in the correct form
static bool MkaooaIsValidMoveString(ReadOnlyString move_string) {
    if (move_string[0] < '1' || move_string[0] > '5') {
        return false;
    } else if (move_string[1] < 'a' || move_string[1] > 'e') {
        return false;
    } else if (move_string[2] < '1' || move_string[2] > '5') {
        return false;
    } else if (move_string[3] < 'a' || move_string[3] > 'e') {
        return false;
    }
    return true;
}

// TODO: Converts the string move a user entered into a Move gamesmanone can understand internally. 
static Move MkaooaStringToMove(ReadOnlyString move_string) {
    assert(MkaooaIsValidMoveString(move_string));

    int fromRow = move_string[0] - '1';
    int fromCol = move_string[1] - 'a';
    int from = fromRow * sideLength + fromCol;

    int toRow = move_string[2] - '1';
    int toCol = move_string[3] - 'a';
    int to = toRow * sideLength + toCol;

    return MOVE_ENCODE(from, to);
}

// MB TODO: Can we just use All Queens Chess's unhash move and hash move? 
//If not, consult Robert/Quixo
static void UnhashMove(Move move, int *from, int *to) {
    *from = move >> 5;
    *to = move & 0x1F;
}
