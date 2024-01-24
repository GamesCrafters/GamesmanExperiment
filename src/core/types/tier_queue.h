/**
 * @file tier_queue.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier queue.
 *
 * @version 1.0.0
 * @date 2024-01-24
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_QUEUE_H
#define GAMESMANONE_CORE_TYPES_TIER_QUEUE_H

#include "core/data_structures/int64_queue.h"
#include "core/types/base.h"

/** @brief Dynamic Tier queue using Int64Queue. */
typedef Int64Queue TierQueue;

/** @brief Initializes QUEUE. */
void TierQueueInit(TierQueue *queue);

/** @brief Destroys QUEUE. */
void TierQueueDestroy(TierQueue *queue);

/** @brief Returns true if QUEUE is empty, or false otherwise. */
bool TierQueueIsEmpty(const TierQueue *queue);

/** @brief Returns the number of items in QUEUE. */
int64_t TierQueueSize(const TierQueue *queue);

/**
 * @brief Pushes TIER into the QUEUE.
 * 
 * @return true on success,
 * @return false otherwise.
 */
bool TierQueuePush(TierQueue *queue, Tier tier);

/** @brief Pops the item at the front of the QUEUE and returns it. */
Tier TierQueuePop(TierQueue *queue);

#endif  // GAMESMANONE_CORE_TYPES_TIER_QUEUE_H
