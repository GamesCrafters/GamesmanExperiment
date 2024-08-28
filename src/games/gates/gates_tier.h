#ifndef GAMESMANONE_GAMES_GATES_GATES_TIER_H_
#define GAMESMANONE_GAMES_GATES_GATES_TIER_H_

#include <inttypes.h>  // PRIu8
#include <stdint.h>    // uint8_t

#include "core/types/gamesman_types.h"

enum {
    kBoardSize = 18,
};

typedef uint8_t GatesTierField;

enum GatesPhase {
    kPlacement,
    kMovement,
    kGate1Moving,
    kGate2Moving,
};

enum GatesPieces { G, g, A, a, Z, z, kNumPieceTypes };

typedef struct {
    GatesTierField n[kNumPieceTypes];  // [0, 2] each
    GatesTierField phase;              // [0, 3]
    GatesTierField G1;                 // [0, 17]
    GatesTierField G2;                 // [G1 + 1, 17]
} GatesTier;

Tier GatesTierHash(const GatesTier *t);
void GatesTierUnhash(Tier hash, GatesTier *dest);

GatesTierField GatesTierGetNumPieces(const GatesTier *t);

Tier GatesGetInitialTier(void);
Tier GatesGetCanonicalTier(Tier tier);
TierArray GatesGetChildTiers(Tier tier);
int GatesGetTierName(Tier tier, char name[static kDbFileNameLengthMax]);

#endif  // GAMESMANONE_GAMES_GATES_GATES_TIER_H_
